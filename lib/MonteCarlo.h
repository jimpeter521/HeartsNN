#pragma once

#include "lib/Strategy.h"
#include "lib/Annotator.h"
#include "lib/GameOutcome.h"
#include "dlib/threads.h"

class KnowableState;

enum ScoreType {
  // We support three different scoring variants.
  // All three are designed so that the sum across the four players for one game hand is zero.

  kBoringScore = 0,
    // The score for a boring game of hearts, i.e. one without shooting the moon.
    // This score is in the range of -6.5 to +19.5.

  kStandardScore = 1,
    // The standard score, taking into account shooting the moon.
    // This score is in the range of -19.5 to 18.5.

  kNumScoreTypes = 2,
    // This is not a score type, but instead is the number of different score types.
};

class MonteCarlo : public Strategy
{
public:
  virtual ~MonteCarlo();

  MonteCarlo(const StrategyPtr& intuition, uint32_t numAlternates, bool parallel, const AnnotatorPtr& annotator);

  virtual Card choosePlay(const KnowableState& state, const RandomGenerator& rng) const;

private:

  class Stats
  {
  public:
    Stats(unsigned numLegalPlays=0);

    Stats(const Stats& other)
    : mNumLegalPlays(other.mNumLegalPlays)
    , mTotalAlternates(other.mTotalAlternates)
    {
      memcpy(scores, other.scores, sizeof(scores));
      memcpy(trickWins, other.trickWins, sizeof(trickWins));
      memcpy(moonCounts, other.moonCounts, sizeof(moonCounts));
    }

    void operator=(const Stats& other) {
      mNumLegalPlays = other.mNumLegalPlays;
      mTotalAlternates = other.mTotalAlternates;
      memcpy(scores, other.scores, sizeof(scores));
      memcpy(trickWins, other.trickWins, sizeof(trickWins));
      memcpy(moonCounts, other.moonCounts, sizeof(moonCounts));
    }

    void operator+=(const Stats& other);

    unsigned NumLegalPlays() const { return mNumLegalPlays; }
    unsigned TotalAlternates() const { return mTotalAlternates; }

    void TrackTrickWinner(GameState& next, int iPlay);
    void UntrackTrickWinner(GameState& next);

    void UpdateForGameOutcome(const GameOutcome& outcome, int currentPlayer, int iPlay);

    void FinishedOneAlternate() { ++mTotalAlternates; }

    Card ComputeProbabilities(const CardHand& choices
                            , float moonProb[13][kNumMoonCountKeys+1]
                            , float winsTrickProb[13]
                            , float expectedScore[13]
                            , ScoreType scoreType
                            , float offset=0.0) const;
  private:
    unsigned mNumLegalPlays;

    unsigned mTotalAlternates;

    // scores is the cumulated score across all simulated alternates for each legal play
    float scores[13];

    // trickWins is a count per legal play of the number of times the play wins the trick.
    // We use it to estimate the probability that if we play this card it will take the trick.
    unsigned trickWins[13];

    int moonCounts[13][kNumMoonCountKeys];
       // Counts across all of the rollouts of when one of four significant events related to shooting the moon occured
       // There is a fifth event, which is the common case where points are split without anyone coming close to shooting moon
       // mc[i][0] is I shot the moon,
       // mc[i][1] is other shot the moon
  };

  void PlayOneAlternate(const KnowableState& knowableState
                      , const PossibilityAnalyzer* analyzer
                      , uint128_t possibilityIndex
                      , const CardHand& choices
                      , const RandomGenerator& rng
                      , Stats& stats) const;

  Stats RunRolloutsTask(const KnowableState& knowableState
                     , PossibilityAnalyzer* analyzer
                     , const CardHand& choices
                     , const RandomGenerator& rng
                     , unsigned kNumAlts) const;

  Stats RunParallelTasks(const KnowableState& knowableState
                       , const RandomGenerator& rng
                       , PossibilityAnalyzer* analyzer
                       , const CardHand& choices) const;

private:
  StrategyPtr mIntuition;
  const uint32_t kNumAlternates;
  const int kNumThreads;
  const bool mParallel;
  mutable dlib::thread_pool mThreadPool;
  dlib::mutex mStatsAccumMutex;
};
