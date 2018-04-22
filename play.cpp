#include "lib/DnnModelIntuition.h"
#include "lib/GameState.h"
#include "lib/HumanPlayer.h"
#include "lib/RandomStrategy.h"

#include "lib/math.h"
#include "lib/random.h"
#include "lib/timer.h"

#include <sys/stat.h>
#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>

#include <stdlib.h>

int main(int argc, char** argv)
{
    StrategyPtr opponent(new RandomStrategy());
    StrategyPtr human(new HumanPlayer());
    StrategyPtr players[4];
    players[0] = human;
    players[1] = players[2] = players[3] = opponent;

    RandomGenerator rng;
    GameState state;
    GameOutcome outcome = state.PlayGame(players, rng);

    for (int p = 0; p < 4; ++p)
        printf("%.1f ", outcome.ZeroMeanStandardScore(p));
    printf("\n");

    return 0;
}
