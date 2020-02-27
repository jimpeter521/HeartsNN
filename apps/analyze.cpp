#include "lib/DnnMonteCarloAnnotator.h"
#include "lib/GameState.h"
#include "lib/math.h"
#include "lib/random.h"
#include "lib/timer.h"

#include <algorithm>
#include <getopt.h>
#include <string>

RandomGenerator generator;

const uint128_t kZero = 0;
const uint128_t kUnset = ~kZero;

StrategyPtr gOpponent;
StrategyPtr gChampion;
uint128_t gDeal = kUnset;

using namespace std;

void usage()
{
    const char* lines[] = {"Usage: analyze [--opponent <strategy>]  [--champion <strategy>] [--deal <hexIndex>]",
        "  <strategy> may be either of the strings ['simple', 'random'] or a path to a saved model directory",
        "  If unspecified, the default <strategy> for opponent is 'simple', and for champion is 'savedmodel'",
        "  <hexIndex> is a hex string of a deal index. If not specified, a random deal will be used.", 0};
    for (int i = 0; lines[i] != 0; ++i)
        printf("%s\n", lines[i]);
    exit(0);
}

void parseArgs(int argc, char** argv)
{
    const struct option longopts[]
        = {{"opponent", required_argument, NULL, 'o'}, {"champion", required_argument, NULL, 'c'},
            {"deal", required_argument, NULL, 'd'}, {"help", no_argument, NULL, 'h'}, {NULL, 0, NULL, 0}};

    while (true)
    {

        int longindex = 0;
        int ch = getopt_long(argc, argv, "o:c:d:h", longopts, &longindex);
        if (ch == -1)
        {
            break;
        }

        switch (ch)
        {
        case 'o':
        {
            printf("Setting opponent to %s\n", optarg);
            gOpponent = makePlayer(optarg);
            break;
        }
        case 'c':
        {
            printf("Setting champion to %s\n", optarg);
            gChampion = makePlayer(optarg);
            break;
        }
        case 'd':
        {
            gDeal = parseHex128(optarg);
            break;
        }
        case 'h':
        default:
        {
            usage();
            break;
        }
        }
    }

    if (gDeal == kUnset)
    {
        gDeal = Deal::RandomDealIndex();
    }

    if (!gOpponent)
    {
        const char* def = "random#500";
        printf("Setting opponent to %s\n", def);
        gOpponent = makePlayer(def);
    }
    if (!gChampion)
    {
        const char* def = "savedmodel";
        printf("Setting champion to %s\n", def);
        gChampion = makePlayer(def);
    }
}

typedef StrategyPtr Player;
typedef StrategyPtr Table[4];

void runGame(StrategyPtr players[4])
{
    std::string asHex = asHexString(gDeal);
    printf("%s\n", asHex.c_str());
    Deal deck(gDeal);
    deck.printDeal();

    GameState state(deck);
    GameOutcome outcome = state.PlayGame(players, generator);

    if (outcome.shotTheMoon())
    {
        printf("Shot the moon!\n");
    }
    printf("Final scores: %3.1f %3.1f %3.1f %3.1f\n", outcome.ZeroMeanStandardScore(0),
        outcome.ZeroMeanStandardScore(1), outcome.ZeroMeanStandardScore(2), outcome.ZeroMeanStandardScore(3));
}

int main(int argc, char** argv)
{
    parseArgs(argc, argv);

    StrategyPtr players[] = {gChampion, gOpponent, gOpponent, gOpponent};
    runGame(players);

    return 0;
}
