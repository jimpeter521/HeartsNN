#include "lib/Deal.h"
#include "lib/GameState.h"
#include "lib/HumanPlayer.h"
#include "lib/Tournament.h"
#include "lib/random.h"

#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char** argv)
{
    const char* opponentName = argc > 1 ? argv[1] : "random#100";

    StrategyPtr opponent = makePlayer(opponentName);
    StrategyPtr human(new HumanPlayer(opponent));
    StrategyPtr players[4];
    players[0] = human;
    players[1] = players[2] = players[3] = opponent;

    RandomGenerator rng;

    float totalScores[4] = {0};
    while (true)
    {
        printf("\n\nNew game...\n");
        uint128_t N = Deal::RandomDealIndex();
        GameState state(N);
        GameOutcome outcome = state.PlayGame(players, rng);
        float humanScore = outcome.ZeroMeanStandardScore(0);

        printf("This hand:\n");
        for (int p = 0; p < 4; ++p)
        {
            float score = outcome.ZeroMeanStandardScore(p);
            totalScores[p] += score;
            printf("%.1f ", score);
        }
        printf("\n");

        Deal deal(N);
        deal.printDeal();

        {
            StrategyPtr _players[4] = {opponent, opponent, opponent, opponent};
            GameState state(deal);
            GameOutcome outcome = state.PlayGame(_players, rng);
            float modelScore = outcome.ZeroMeanStandardScore(0);
            printf("Your score: %.1f\n", humanScore);
            printf("Model score: %.1f\n", modelScore);
            if (humanScore < modelScore)
            {
                printf("Well done!\n");
            }
        }

        printf("\nTotals so far:\n");
        for (int p = 0; p < 4; ++p)
        {
            printf("%.1f ", totalScores[p]);
        }
        printf("\n");
    }

    return 0;
}
