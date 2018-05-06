// lib/Tournament.h

#pragma once

#include "lib/GameOutcome.h"
#include "lib/Strategy.h"
#include "lib/random.h"

struct Scores;

class Tournament
{
public:
    Tournament(StrategyPtr champion, StrategyPtr opponent, bool quiet = false, bool saveMoonDeals = false);

    float runOneTournament(int numMatches = 1, uint128_t* gDeals = nullptr);

    void runOneMatch(const uint128_t dealIndex, float playerScores[2]);

    void runOneGame(uint128_t dealIndex, StrategyPtr players[4], Scores& scores, bool& moon);

private:
    StrategyPtr mChampion;
    StrategyPtr mOpponent;
    bool mQuiet;
    bool mSaveMoonDeals;
    RandomGenerator mRng;
};

// This Scores struct is useful for analyzing the results of one match.
// It accumulates both total scores for each player strategy, but also
// the scores for each position at the table across the six games in the match.
struct Scores
{
    StrategyPtr mChampion;
    float mPlayer[2];
    float mPosition[4];
    float mCross[2][4];

    Scores(StrategyPtr champion)
        : mChampion(champion)
    {
        std::fill(mPlayer, mPlayer + 2, 0.0);
        std::fill(mPosition, mPosition + 4, 0.0);
        std::fill(&mCross[0][0], &mCross[2][0], 0.0);
    }

    void Accumulate(StrategyPtr players[4], const GameOutcome& outcome)
    {
        for (int j = 0; j < 4; ++j)
        {
            StrategyPtr player = players[j];
            int playerIndex = player == mChampion ? 0 : 1;
            mPlayer[playerIndex] += outcome.ZeroMeanStandardScore(j);
            mPosition[j] += outcome.ZeroMeanStandardScore(j);
            mCross[playerIndex][j] += outcome.ZeroMeanStandardScore(j);
        }
    }

    void Summarize() const
    {
        const char* name[2] = {"c", "o"};
        printf("\n");
        for (int i = 0; i < 2; ++i)
        {
            printf("%s ", name[i]);
            for (int j = 0; j < 4; ++j)
            {
                printf("%5.1f ", mCross[i][j] / 3.0);
            }
            printf("| %5.1f\n", mPlayer[i] / 6.0);
        }
        for (int j = 0; j < 4; ++j)
        {
            printf("%5.1f ", mPosition[j] / 6.0);
        }
        printf("\n\n");
    }
};
