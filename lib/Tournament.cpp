// lib/Tournament.cpp

#include "lib/Tournament.h"
#include "lib/GameState.h"

typedef StrategyPtr Player;
typedef StrategyPtr Table[4];

Tournament::Tournament(StrategyPtr champion, StrategyPtr opponent, bool quiet, bool saveMoonDeals)
    : mChampion(champion)
    , mOpponent(opponent)
    , mQuiet(quiet)
    , mSaveMoonDeals(saveMoonDeals)
{}

void Tournament::runOneGame(uint128_t dealIndex, StrategyPtr players[4], Scores& scores, bool& moon)
{
    Deal deck(dealIndex);
    GameState state(deck);
    GameOutcome outcome = state.PlayGame(players, mRng);
    moon = outcome.shotTheMoon();

    const char* name[2] = {"c", "o"};

    scores.Accumulate(players, outcome);

    if (!mQuiet)
    {
        for (int i = 0; i < 4; ++i)
        {
            StrategyPtr player = players[i];
            int playerIndex = player == mChampion ? 0 : 1;
            printf("%s=%5.1f ", name[playerIndex], outcome.ZeroMeanStandardScore(i));
        }
        if (moon)
            printf("  Shot the moon!\n");
        else
            printf("\n");
    }
}

void Tournament::runOneMatch(const uint128_t dealIndex, float playerScores[2])
{

    // A match is six games with the same deal of cards to the four positions (N, E, S, W)
    // The two player strategies each occupy two of the table positions.
    // There are six unique arrangements of the two player strategies.
    // We'll rollup all of the game scores into one score for each strategy.
    Table match[6] = {
        {mChampion, mChampion, mOpponent, mOpponent},
        {mChampion, mOpponent, mChampion, mOpponent},
        {mChampion, mOpponent, mOpponent, mChampion},
        {mOpponent, mOpponent, mChampion, mChampion},
        {mOpponent, mChampion, mOpponent, mChampion},
        {mOpponent, mChampion, mChampion, mOpponent},
    };

    Scores matchScores(mChampion);

    int shotMoon = 0;

    Deal deck(dealIndex);
    if (!mQuiet)
    {
        std::string asHex = asHexString(dealIndex);
        printf("%s\n", asHex.c_str());
        deck.printDeal();
    }

    for (int i = 0; i < 6; ++i)
    {
        bool moon;
        runOneGame(dealIndex, match[i], matchScores, moon);
        if (moon)
            ++shotMoon;
    }

    if (!mQuiet)
    {
        matchScores.Summarize();
    }

    for (int p = 0; p < 2; ++p)
        playerScores[p] += matchScores.mPlayer[p];

    if (mSaveMoonDeals && shotMoon > 1)
    {
        std::string hex = asHexString(dealIndex);
        FILE* f = fopen("moonhands.txt", "a");
        fprintf(f, "%s\n", hex.c_str());
        fclose(f);
    }
}

float Tournament::runOneTournament(int numMatches, uint128_t* deals)
{
    float playerScores[2] = {0};

    for (int i = 0; i < numMatches; ++i)
    {
        runOneMatch(deals[i], playerScores);
    }

    if (!mQuiet)
    {
        printf("Champion: %4.2f\n", playerScores[0] / (numMatches * 6.0));
        printf("Opponent: %4.2f\n", playerScores[1] / (numMatches * 6.0));
        if (playerScores[0] <= playerScores[1])
        {
            printf("Champion wins\n");
        }
        else
        {
            printf("Oppponent wins\n");
        }
    }

    return playerScores[0] / (numMatches * 6.0); // the champ's score
}
