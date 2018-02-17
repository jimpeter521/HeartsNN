#include "lib/Card.h"
#include "lib/GameState.h"
#include "lib/KnowableState.h"
#include "lib/random.h"
#include "lib/RandomStrategy.h"
#include "lib/MonteCarlo.h"
#include "lib/PossibilityAnalyzer.h"
#include "lib/timer.h"

#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <algorithm>

static RandomGenerator rng;

MonteCarlo::~MonteCarlo() {
}

MonteCarlo::MonteCarlo(const StrategyPtr& intuition
                     , uint32_t minAlternates
                     , uint32_t maxAlternates
                     , float timeBudget
                     , const AnnotatorPtr& annotator)
: Strategy(annotator)
, mIntuition(intuition)
, kMinAlternates(minAlternates)
, kMaxAlternates(maxAlternates)
, kTimeBudget(timeBudget)
{
  assert(4 < kMinAlternates);
  assert(kMinAlternates <= kMaxAlternates);
  assert(kMaxAlternates <= 2000);
  assert(0.05 <= timeBudget);
  assert(timeBudget <= 1.0);
}

// For each legal play, play out (roll out) the game many times
// Compute the expected score of a play as the average score all game rollouts.

Card MonteCarlo::choosePlay(const KnowableState& knowableState) const
{
  // knowableState.VerifyHeartsState();

  const unsigned currentPlayer = knowableState.CurrentPlayer();
  const CardHand choices = knowableState.LegalPlays();

  if (choices.Size()==1)
    return choices.FirstCard();

  assert(knowableState.PointsPlayed() < 26);

  float scores[13] = {0.0};

  // trickWins is a count per legal play of the number of times the play wins the trick.
  // We use it to estimate the probability that if we play this card it will take the trick.
  unsigned trickWins[13] = {0};

  int moonCounts[13][4] = {{0}};
     // Counts across all of the rollouts of when one of four significant events related to shooting the moon occured
     // There is a fifth event, which is the common case where points are split without anyone coming close to shooting moon
     // mc[i][0] is I shot the moon,
     // mc[i][1] is other shot the moon
     // mc[i][2] is I stopped other from shooting the moon
     // mc[i][3] is other stopped me from shooting the moon

  PossibilityAnalyzer* analyzer = knowableState.Analyze();
  const uint128_t numPossibilities = analyzer->Possibilities();

  // const uint64_t estimatedworkunits =  choices.Size() * (48 - knowableState.PlayNumber());
  double start = now();

  // For each possible alternate arrangement of opponent's unplayed cards
  unsigned alternate;
  for (alternate=0; alternate<kMaxAlternates; ++alternate)
  {
    const uint128_t possibilityIndex = rng.range128(numPossibilities);

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
      next.TrackTrickWinner(trickWins + i);
      next.PlayCard(nextCardPlayed);

      // Do one "roll out", i.e. play out the game to the end, using random plays
      GameOutcome outcome = next.PlayOutGameMonteCarlo(mIntuition);

      next.TrackTrickWinner(0);

      outcome.updateMoonStats(currentPlayer, i, moonCounts);

      scores[i] += outcome.boringScore(currentPlayer);
    }

    if (alternate>=kMinAlternates && delta(start) > kTimeBudget) {
      // printf("Play %u, choices %u, stopped at %3.2f\n", knowableState.PlayNumber(), choices.Size(), 100.0*float(alternate)/kMaxAlternates);
      break;
    }
  }

  // if (alternate == kMaxAlternates) {
  //   printf("Not stopped: Play %u, choices %u\n", knowableState.PlayNumber(), choices.Size());
  // }

  enum MoonCountKey {
    kCurrentShotTheMoon = 0,
    kOtherShotTheMoon = 1,
    kCurrentStoppedTheMoon = 2,
    kOtherStoppedTheMoon = 3,
    kNumMoonCountKeys = 4
  };

  const unsigned totalAlternates = alternate;
  const float kScale = 1.0 / totalAlternates;
  float moonProb[13][5];
  float winsTrickProb[13];
  for (unsigned i=0; i<choices.Size(); ++i) {
    int notMoonCount = totalAlternates - (moonCounts[i][0] + moonCounts[i][1] + moonCounts[i][2] + moonCounts[i][3]);
    moonProb[i][kCurrentShotTheMoon] = moonCounts[i][kCurrentShotTheMoon] * kScale;
    moonProb[i][kOtherShotTheMoon] = moonCounts[i][kOtherShotTheMoon] * kScale;
    moonProb[i][kCurrentStoppedTheMoon] = moonCounts[i][kCurrentStoppedTheMoon] * kScale;
    moonProb[i][kOtherStoppedTheMoon] = moonCounts[i][kOtherStoppedTheMoon] * kScale;
    moonProb[i][4] = notMoonCount * kScale;

    winsTrickProb[i] = trickWins[i] * kScale;
  }

  unsigned bestChoice = 0;
  float bestScore = 1e10;
  float expectedScore[choices.Size()];
  for (unsigned i=0; i<choices.Size(); ++i) {
    float score = scores[i] * kScale;
    expectedScore[i] = score;
    if (bestScore > score) {
      bestScore = score;
      bestChoice = i;
    }
  }

  Card bestPlay = choices.NthCard(bestChoice);

  const AnnotatorPtr annotator = getAnnotator();
  if (annotator)
    annotator->OnWriteData(knowableState, analyzer, expectedScore, moonProb, winsTrickProb);

  delete analyzer;
  return bestPlay;
}
