#include "lib/Card.h"
#include "lib/GameState.h"
#include "lib/KnowableState.h"
#include "lib/random.h"
#include "lib/RandomStrategy.h"
#include "lib/MonteCarlo.h"
#include "lib/PossibilityAnalyzer.h"
#include "lib/timer.h"

#include "dlib/logger.h"

using namespace dlib;

static logger dlog("MonteCarlo");

#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <algorithm>

MonteCarlo::~MonteCarlo() {
}

MonteCarlo::MonteCarlo(const StrategyPtr& intuition
                     , uint32_t numAlternates
                     , bool parallel
                     , const AnnotatorPtr& annotator)
: Strategy(annotator)
, mIntuition(intuition)
, kNumAlternates(numAlternates)
, kNumThreads(parallel ? std::thread::hardware_concurrency()/2 : 0)
, mParallel(parallel)
, mThreadPool(kNumThreads)
{
  dlog.set_level(LALL);
}

void MonteCarlo::PlayOneAlternate(const KnowableState& knowableState
                                , const PossibilityAnalyzer* analyzer
                                , uint128_t possibilityIndex
                                , const CardHand& choices
                                , const RandomGenerator& rng
                                , Stats& stats) const
{
  const unsigned currentPlayer = knowableState.CurrentPlayer();

  CardHands hands;
  knowableState.PrepareHands(hands);
  analyzer->ActualizePossibility(possibilityIndex, hands);

  knowableState.IsVoidBits().VerifyVoids(hands);

  // Construct the game state for this alternate
  const GameState alt(hands, knowableState);

  CardArray::iterator it(choices);

  // For each possible play
  for (unsigned i=0; i<choices.Size(); ++i)
  {
    Card nextCardPlayed = it.next();

    // Construct the next game state
    GameState next(alt);
    stats.TrackTrickWinner(next, i);
    next.PlayCard(nextCardPlayed);

    // Do one "roll out", i.e. play out the game to the end, using random plays
    GameOutcome outcome = next.PlayOutGameMonteCarlo(mIntuition, rng);

    stats.UntrackTrickWinner(next);
    stats.UpdateForGameOutcome(outcome, currentPlayer, i);
  }

  stats.FinishedOneAlternate();
}

MonteCarlo::Stats MonteCarlo::RunRolloutsTask(const KnowableState& knowableState
                               , PossibilityAnalyzer* analyzer
                               , const CardHand& choices
                               , const RandomGenerator& rng
                               , unsigned kNumAlts) const
{
  const uint128_t numPossibilities = analyzer->Possibilities();
  Stats thisTaskStats(choices.Size());
  for (unsigned alternate = 0; alternate<kNumAlts; ++alternate)
  {
    const uint128_t possibilityIndex = rng.range128(numPossibilities);
    PlayOneAlternate(knowableState, analyzer, possibilityIndex, choices, rng, thisTaskStats);
  }

  return thisTaskStats;
}

MonteCarlo::Stats MonteCarlo::RunParallelTasks(const KnowableState& knowableState
                                , const RandomGenerator& rng
                                , PossibilityAnalyzer* analyzer
                                , const CardHand& choices) const
{
  Stats totalStats(choices.Size());

  // double n = now();

  assert(kNumThreads >= 1);
  const unsigned kNumAlts = (kNumAlternates+kNumThreads-1) / kNumThreads;

  std::vector<std::future<Stats>> taskStats(kNumThreads);

  for (int i=0; i<kNumThreads; ++i)
  {
    taskStats[i] = dlib::async(mThreadPool, [this, knowableState, analyzer, choices, rng, kNumAlts]()
        {return this->RunRolloutsTask(knowableState, analyzer, choices, rng, kNumAlts);
    });
  }

  for (int i=0; i<kNumThreads; ++i)
    totalStats += taskStats[i].get();

  // dlog << LINFO << "Computed " << totalStats.TotalAlternates() << " in secs: " << delta(n);
  return totalStats;
}

// For each legal play, play out (roll out) the game many times
// Compute the expected score of a play as the average score all game rollouts.
Card MonteCarlo::choosePlay(const KnowableState& knowableState, const RandomGenerator& rng) const
{
  // knowableState.VerifyHeartsState();

  const CardHand choices = knowableState.LegalPlays();

  if (choices.Size()==1)
    return choices.FirstCard();

  assert(knowableState.PointsPlayed() < 26);

  PossibilityAnalyzer* analyzer = knowableState.Analyze();

  Stats totalStats;
  if (!mParallel)
  {
    totalStats = this->RunRolloutsTask(knowableState, analyzer, choices, rng, kNumAlternates);
  }
  else
  {
    totalStats = RunParallelTasks(knowableState, rng, analyzer, choices);
  }

  float moonProb[13][3];
  float winsTrickProb[13];
  float expectedScore[13];

  // TODO: Better name, this does more than compute probabilities.
  Card bestPlay = totalStats.ComputeProbabilities(choices, moonProb, winsTrickProb, expectedScore, kStandardScore);

  const AnnotatorPtr annotator = getAnnotator();
  if (annotator) {
    float offset = float(knowableState.GetScoreFor(knowableState.CurrentPlayer()));
    totalStats.ComputeProbabilities(choices, moonProb, winsTrickProb, expectedScore, kBoringScore, offset);
    annotator->OnWriteData(knowableState, analyzer, expectedScore, moonProb, winsTrickProb);
  }

  delete analyzer;
  return bestPlay;
}

MonteCarlo::Stats::Stats(unsigned numLegalPlays)
: mNumLegalPlays(numLegalPlays)
, mTotalAlternates(0)
{
  bzero(scores, sizeof(scores));
  bzero(trickWins, sizeof(trickWins));
  bzero(moonCounts, sizeof(moonCounts));
}

void MonteCarlo::Stats::UpdateForGameOutcome(const GameOutcome& outcome, int currentPlayer, int iPlay) {
  outcome.updateMoonStats(currentPlayer, iPlay, moonCounts);
  scores[iPlay] += outcome.boringScore(currentPlayer);
}

void MonteCarlo::Stats::TrackTrickWinner(GameState& next, int iPlay) {
  next.TrackTrickWinner(trickWins + iPlay);
}

void MonteCarlo::Stats::UntrackTrickWinner(GameState& next) {
  next.TrackTrickWinner(0);
}

const float kScoreTypeOffsets[kNumScoreTypes][kNumMoonCountKeys] = {
  { 0.0, 0.0 },     // kBoringScore
  { -39.5, 13.0 },   // kStandardScore
};

Card MonteCarlo::Stats::ComputeProbabilities(const CardHand& choices
                        , float moonProb[13][kNumMoonCountKeys+1]
                        , float winsTrickProb[13]
                        , float expectedScore[13]
                        , ScoreType scoreType
                        , float offset) const
{
  const float kScale = 1.0 / mTotalAlternates;
  for (unsigned i=0; i<choices.Size(); ++i) {
    int notMoonCount = mTotalAlternates - (moonCounts[i][0] + moonCounts[i][1]);
    moonProb[i][kCurrentShotTheMoon] = moonCounts[i][kCurrentShotTheMoon] * kScale;
    moonProb[i][kOtherShotTheMoon] = moonCounts[i][kOtherShotTheMoon] * kScale;
    moonProb[i][2] = notMoonCount * kScale;

    winsTrickProb[i] = trickWins[i] * kScale;
  }

  assert(scoreType>=kBoringScore);
  assert(scoreType<kNumScoreTypes);
  const float* kScoreOffset = kScoreTypeOffsets[scoreType];

  const float kEpsilon = 0.001;  // a little fudge factor for inexact floating point.
  unsigned bestChoice = 0;
  float bestScore = 1e10;
  for (unsigned i=0; i<choices.Size(); ++i) {
    float score = scores[i] * kScale;

    if (scoreType == kStandardScore)
      score -= 6.5;

    for (unsigned j=0; j<kNumMoonCountKeys; ++j) {
      score += kScoreOffset[j] * moonProb[i][j];
    }

    if (scoreType == kBoringScore) {
      assert(score >= 0.0   - kEpsilon);
      assert(score <= 26.0  + kEpsilon);
    } else {
      assert(scoreType == kStandardScore);
      assert(score >= -19.5 - kEpsilon);
      assert(score <=  18.5 + kEpsilon);
    }

    expectedScore[i] = (score - offset) / 26.0;
    if (bestScore > score) {
      bestScore = score;
      bestChoice = i;
    }
  }

  return choices.NthCard(bestChoice);
}

void MonteCarlo::Stats::operator+=(const MonteCarlo::Stats& other)
{
  assert(mNumLegalPlays == other.mNumLegalPlays);
  mTotalAlternates += other.mTotalAlternates;
  for (unsigned i=0; i<mNumLegalPlays; ++i)
  {
    scores[i] += other.scores[i];
    trickWins[i] += other.trickWins[i];

    for (unsigned j=0; j<4; ++j) {
      moonCounts[i][j] += other.moonCounts[i][j];
    }
  }
}
