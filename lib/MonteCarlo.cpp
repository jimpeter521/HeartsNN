#include "lib/MonteCarlo.h"
#include "lib/Card.h"
#include "lib/DebugStats.h"
#include "lib/GameState.h"
#include "lib/KnowableState.h"
#include "lib/PossibilityAnalyzer.h"
#include "lib/RandomStrategy.h"
#include "lib/random.h"
#include "lib/timer.h"

#include "dlib/logger.h"

using namespace dlib;

static logger dlog("MonteCarlo");

#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>

MonteCarlo::~MonteCarlo() {}

MonteCarlo::MonteCarlo(
    const StrategyPtr& intuition, uint32_t numAlternates, bool parallel, const AnnotatorPtr& annotator)
    : Strategy(annotator)
    , mIntuition(intuition)
    , kNumAlternates(numAlternates)
    , kNumThreads(parallel ? (3 * std::thread::hardware_concurrency()) / 4 : 0)
    , mParallel(parallel)
    , mThreadPool(kNumThreads)
{
    dlog.set_level(LALL);
}

void MonteCarlo::PlayOneAlternate(const KnowableState& knowableState, const PossibilityAnalyzer* analyzer,
    uint128_t possibilityIndex, const CardHand& choices, const RandomGenerator& rng, Stats& stats) const
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
    for (unsigned i = 0; i < choices.Size(); ++i)
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

MonteCarlo::Stats MonteCarlo::RunRolloutsTask(const KnowableState& knowableState, PossibilityAnalyzer* analyzer,
    const CardHand& choices, const RandomGenerator& rng, unsigned kNumAlts) const
{
    const uint128_t numPossibilities = analyzer->Possibilities();
    Stats thisTaskStats(choices.Size());
    for (unsigned alternate = 0; alternate < kNumAlts; ++alternate)
    {
        const uint128_t possibilityIndex = rng.range128(numPossibilities);
        PlayOneAlternate(knowableState, analyzer, possibilityIndex, choices, rng, thisTaskStats);
    }

    return thisTaskStats;
}

MonteCarlo::Stats MonteCarlo::RunParallelTasks(const KnowableState& knowableState, const RandomGenerator& rng,
    PossibilityAnalyzer* analyzer, const CardHand& choices) const
{
    Stats totalStats(choices.Size());

    // double n = now();

    assert(kNumThreads >= 1);
    const unsigned kNumAlts = (kNumAlternates + kNumThreads - 1) / kNumThreads;

    std::vector<std::future<Stats>> taskStats(kNumThreads);

    for (int i = 0; i < kNumThreads; ++i)
    {
        taskStats[i] = dlib::async(mThreadPool, [this, i, knowableState, analyzer, choices, kNumAlts]() {
            const RandomGenerator& rng = RandomGenerator::ThreadSpecific();
            return this->RunRolloutsTask(knowableState, analyzer, choices, rng, kNumAlts);
        });
    }

    for (int i = 0; i < kNumThreads; ++i)
        totalStats += taskStats[i].get();

    // dlog << LINFO << "Computed " << totalStats.TotalAlternates() << " in secs: " << delta(n);
    return totalStats;
}

Card MonteCarlo::predictOutcomes(
    const KnowableState& state, const RandomGenerator& rng, float playExpectedValue[13]) const
{
    // TODO: actually do the rollouts and return the better predictions
    return mIntuition->predictOutcomes(state, rng, playExpectedValue);
}

// For each legal play, play out (roll out) the game many times
// Compute the expected score of a play as the average score all game rollouts.
Card MonteCarlo::choosePlay(const KnowableState& knowableState, const RandomGenerator& rng) const
{
    // knowableState.VerifyHeartsState();

    const CardHand choices = knowableState.LegalPlays();

    if (choices.Size() == 1)
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

    const AnnotatorPtr annotator = getAnnotator();
    if (annotator)
    {
        float moonProb[13][3];
        float winsTrickProb[13];
        float expectedDelta[13];
        const unsigned currentPoints = knowableState.GetScoreFor(knowableState.CurrentPlayer());
        totalStats.ComputeTargetValues(choices, moonProb, winsTrickProb, expectedDelta, currentPoints);
        annotator->OnWriteData(knowableState, analyzer, expectedDelta, moonProb, winsTrickProb);
    }

    delete analyzer;

    Card bestPlay = totalStats.BestPlay(choices);
    return bestPlay;
}

// mTotalPoints is the cumulated points across all simulated alternates for each legal play
// points come directly from GameState where they are in the range of 0..26.
unsigned mTotalPoints[13];

// trickWins is a count per legal play of the number of times the play wins the trick.
// We use it to estimate the probability that if we play this card it will take the trick.
unsigned mTotalTrickWins[13];

unsigned mTotalMoonCounts[13][kNumMoonCountKeys];

DebugStats _UpdateForGameOutcome("MonteCarlo::Stats::UpdateForGameOutcome");
DebugStats _expectedPoints("MonteCarlo::expectedPoints");
DebugStats _expectedDelta("MonteCarlo::expectedDelta");
DebugStats _standardScoreStats("MonteCarlo::standard");

MonteCarlo::Stats::Stats(unsigned numLegalPlays)
    : mNumLegalPlays(numLegalPlays)
    , mTotalAlternates(0)
{
    bzero(mTotalPoints, sizeof(mTotalPoints));
    bzero(mTotalTrickWins, sizeof(mTotalTrickWins));
    bzero(mTotalMoonCounts, sizeof(mTotalMoonCounts));
}

void MonteCarlo::Stats::UpdateForGameOutcome(const GameOutcome& outcome, int currentPlayer, int iPlay)
{
    outcome.updateMoonStats(currentPlayer, iPlay, mTotalMoonCounts);
    unsigned pointsTaken = outcome.PointsTaken(currentPlayer);
    _UpdateForGameOutcome.Accum(float(pointsTaken));
    mTotalPoints[iPlay] += pointsTaken;
}

void MonteCarlo::Stats::TrackTrickWinner(GameState& next, int iPlay) { next.TrackTrickWinner(mTotalTrickWins + iPlay); }

void MonteCarlo::Stats::UntrackTrickWinner(GameState& next) { next.TrackTrickWinner(0); }

Card MonteCarlo::Stats::BestPlay(const CardHand& choices) const
{
    float moonProb[kCardsPerHand][kNumMoonCountKeys];
    float winsTrickProb[kCardsPerHand];
    const float kScale = 1.0 / mTotalAlternates;
    for (unsigned i = 0; i < choices.Size(); ++i)
    {
        moonProb[i][kCurrentShotTheMoon] = mTotalMoonCounts[i][kCurrentShotTheMoon] * kScale;
        moonProb[i][kOtherShotTheMoon] = mTotalMoonCounts[i][kOtherShotTheMoon] * kScale;
        winsTrickProb[i] = mTotalTrickWins[i] * kScale;
    }

    unsigned bestChoice = 0;
    float bestScore = 1e10;
    for (unsigned i = 0; i < choices.Size(); ++i)
    {
        float expectedPoints = mTotalPoints[i] * kScale;

        assert(expectedPoints >= 0.0);
        assert(expectedPoints <= 26.0);
        _expectedPoints.Accum(expectedPoints);

        float score = expectedPoints - 6.5; // subtract out the mean score for the typical non-moon outcome
        score -= 39.0 * moonProb[i][kCurrentShotTheMoon];
        score += 13.0 * moonProb[i][kOtherShotTheMoon];

        _standardScoreStats.Accum(score);
        assert(score >= -19.5);
        assert(score <= 18.5);

        if (bestScore > score)
        {
            bestScore = score;
            bestChoice = i;
        }
    }

    return choices.NthCard(bestChoice);
}

void MonteCarlo::Stats::ComputeTargetValues(const CardHand& choices, float moonProb[13][kNumMoonCountKeys + 1],
    float winsTrickProb[13], float expectedDelta[13], unsigned pointsAlreadyTaken) const
{
    const float kScale = 1.0 / mTotalAlternates;
    for (unsigned i = 0; i < choices.Size(); ++i)
    {
        int notMoonCount
            = mTotalAlternates - (mTotalMoonCounts[i][kCurrentShotTheMoon] + mTotalMoonCounts[i][kOtherShotTheMoon]);
        moonProb[i][kCurrentShotTheMoon] = mTotalMoonCounts[i][kCurrentShotTheMoon] * kScale;
        moonProb[i][kOtherShotTheMoon] = mTotalMoonCounts[i][kOtherShotTheMoon] * kScale;
        moonProb[i][2] = notMoonCount * kScale;
        winsTrickProb[i] = mTotalTrickWins[i] * kScale;
    }

    assert(pointsAlreadyTaken < kMaxPointsPerHand);
    // we will never see the max here, as we don't process games states with all point cards already played

    const float kPointsAlreadyTaken = float(pointsAlreadyTaken);

    for (unsigned i = 0; i < choices.Size(); ++i)
    {
        float expectedPoints = mTotalPoints[i] * kScale;

        assert(expectedPoints >= kPointsAlreadyTaken);
        assert(expectedPoints <= float(kMaxPointsPerHand)); // we can (rarely) see all points taken here, when a player
                                                            // can force shooting the moon

        const float kExpectedDelta = expectedPoints - kPointsAlreadyTaken;
        expectedDelta[i] = kExpectedDelta;
        _expectedDelta.Accum(kExpectedDelta);
    }
}

void MonteCarlo::Stats::operator+=(const MonteCarlo::Stats& other)
{
    assert(mNumLegalPlays == other.mNumLegalPlays);
    mTotalAlternates += other.mTotalAlternates;
    for (unsigned i = 0; i < mNumLegalPlays; ++i)
    {
        mTotalPoints[i] += other.mTotalPoints[i];
        mTotalTrickWins[i] += other.mTotalTrickWins[i];

        for (unsigned j = 0; j < kNumMoonCountKeys; ++j)
        {
            mTotalMoonCounts[i][j] += other.mTotalMoonCounts[i][j];
        }
    }
}
