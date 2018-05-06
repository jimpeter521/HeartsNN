#include "lib/Tournament.h"
#include "lib/GameState.h"
#include "lib/MonteCarlo.h"

#include "lib/math.h"
#include "lib/random.h"
#include "lib/timer.h"

#include <algorithm>
#include <getopt.h>
#include <string>

enum PlayerRole
{
    kChampion,
    kOpponent
};

const char* gChampionStr = "random#100";
const char* gOpponentStr = "random";

StrategyPtr gOpponent;
StrategyPtr gChampion;

bool gSaveMoonDeals = true;
bool gQuiet = false;

const char* PlayerName(PlayerRole role) { return role == kChampion ? "Champion" : "Opponent"; }

void usage()
{
    const char* lines[] = {"Usage: tournament [options...]",
        "  Options:", "    -g,--games <int>           the number of games to play (default:1)",
        "    -m,--model <modelPath>     a path to the model to load for `intuion` and `dnnmonte` strategies "
        "(default:savemodel)",
        "    -o,--opponent <strategy>   the strategy to use for the `opponent` (default:random)",
        "    -c,--champion <strategy>   the strategy to use for the `champion` (default: simple)",
        "    -d,--deals <dealIndexFile> a file containing deal indexes to play from (default: choose deals at random)",
        "    -h,--help                  print this message", 0};
    for (int i = 0; lines[i] != 0; ++i)
        printf("%s\n", lines[i]);
    exit(0);
}

int gNumMatches;
uint128_t* gDeals;

const void randomDeals(int n)
{
    gNumMatches = n;
    gDeals = new uint128_t[gNumMatches];
    for (int i = 0; i < gNumMatches; ++i)
        gDeals[i] = Deal::RandomDealIndex();
}

void trim(char* line)
{
    int len = strlen(line);
    while (isspace(line[len - 1]))
    {
        --len;
        line[len] = 0;
    }
}

const void readDeals(const char* path)
{
    const int kMaxLines = 1000;
    gDeals = new uint128_t[kMaxLines];
    FILE* f = fopen(path, "r");
    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    int i = 0;
    while ((linelen = getline(&line, &linecap, f)) > 0)
    {
        trim(line);
        gDeals[i++] = parseHex128(line);
        printf("Using deal %s -> %s\n", line, asHexString(gDeals[i - 1]).c_str());
    }
    gNumMatches = i;
}

void parseArgs(int argc, char** argv)
{
    const struct option longopts[] = {{"model", required_argument, NULL, 'm'}, {"games", required_argument, NULL, 'g'},
        {"opponent", required_argument, NULL, 'o'}, {"champion", required_argument, NULL, 'c'},
        {"deals", required_argument, NULL, 'd'}, {"quiet", no_argument, NULL, 'q'}, {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};

    while (true)
    {

        int longindex = 0;
        int ch = getopt_long(argc, argv, "m:g:o:c:d:qh", longopts, &longindex);
        if (ch == -1)
        {
            break;
        }

        switch (ch)
        {
        case 'o':
        {
            gOpponentStr = optarg;
            break;
        }
        case 'c':
        {
            gChampionStr = optarg;
            break;
        }
        case 'g':
        {
            randomDeals(atoi(optarg));
            break;
        }
        case 'd':
        {
            readDeals(optarg);
            gSaveMoonDeals = false;
            break;
        }
        case 'q':
        {
            gQuiet = true;
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

    if (gDeals == 0)
    {
        randomDeals(1);
    }
}

#if 1
int main(int argc, char** argv)
{
    parseArgs(argc, argv);

    gChampion = makePlayer(gChampionStr);
    gOpponent = makePlayer(gOpponentStr);

    Tournament tournament(gChampion, gOpponent, gQuiet, gSaveMoonDeals);

    tournament.runOneTournament(gNumMatches, gDeals);

    return 0;
}
#else
int main(int argc, char** argv)
{
    const char* modelPath = "pereubu.save/d2w200_relu";
    gQuiet = true;
    gNumMatches = 50;
    randomDeals(gNumMatches);

    for (int rollouts = 1; rollouts <= 51; rollouts += 10)
    {
        // Champion is pure intuition
        gChampion = makePlayer(kChampion, "random", 100);

        // Opponent is monte carlo on random intuition with given number of rollouts
        gOpponent = makePlayer(kOpponent, modelPath, rollouts);

        float champScore = runOneTournament(gChampion, gOpponent);
        printf("%d\t%f\n", rollouts, champScore);
        fflush(stdout);
    }
    return 0;
}
#endif
