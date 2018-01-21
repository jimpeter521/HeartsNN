#include "lib/RandomStrategy.h"
#include "lib/SimpleMonteCarlo.h"
#include "lib/DnnMonteCarlo.h"
#include "lib/GameState.h"
#include "lib/DnnMonteCarloAnnotator.h"

#include "lib/random.h"
#include "lib/math.h"
#include "lib/timer.h"

#include <getopt.h>
#include <string>

#include <algorithm>

#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>

RandomGenerator generator;

const uint128_t kZero = 0;
const uint128_t kUnset = ~kZero;

Strategy* gOpponent = 0;
Strategy* gChampion = 0;
uint128_t gDeal = kUnset;

using namespace std;
using namespace tensorflow;

tensorflow::SavedModelBundle gModel;
Annotator* gAnnotator = 0;

Strategy* makePlayer(const char* arg) {
  Strategy* player = 0;
  if (arg == 0 || std::string(arg) == std::string("random")) {
    player = new RandomStrategy();
  }
  else if (std::string(arg) == std::string("simple")) {
    player = new SimpleMonteCarlo();
  }
  else {
    SessionOptions session_options;
    RunOptions run_options;
    auto status = LoadSavedModel(session_options, run_options, arg, {kSavedModelTagServe}, &gModel);
    if (!status.ok()) {
       std::cerr << "Failed: " << status;
       exit(1);
    }
    player = new DnnMonteCarlo(gModel);
    gAnnotator = new DnnMonteCarloAnnotator(gModel);
  }
  return player;
}

void usage() {
  const char* lines[] = {
    "Usage: analyze [--opponent <strategy>]  [--champion <strategy>] [--deal <hexIndex>]",
    "  <strategy> may be either of the strings ['simple', 'random'] or a path to a saved model directory",
    "  If unspecified, the default <strategy> for opponent is 'simple', and for champion is 'savedmodel'",
    "  <hexIndex> is a hex string of a deal index. If not specified, a random deal will be used.",
    0
  };
  for (int i=0; lines[i]!=0; ++i)
    printf("%s\n", lines[i]);
  exit(0);
}

void parseArgs(int argc, char** argv) {
  const struct option longopts[] = {
    { "opponent", required_argument, NULL, 'o' },
    { "champion", required_argument, NULL, 'c' },
    { "deal", required_argument, NULL, 'd'},
    { "help", no_argument, NULL, 'h' },
    { NULL,                       0, NULL,  0  }
  };

  while (true) {

    int longindex = 0;
    int ch = getopt_long(argc, argv, "o:c:d:h", longopts, &longindex);
    if (ch == -1) {
      break;
    }

    switch (ch) {
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

  if (gDeal == kUnset) {
    gDeal = Deal::RandomDealIndex();
  }

  if (gOpponent == 0) {
    const char* def = "simple";
    printf("Setting opponent to %s\n", def);
    gOpponent = makePlayer(def);
  }
  if (gChampion == 0) {
    const char* def = "savedmodel";
    printf("Setting champion to %s\n", def);
    gChampion = makePlayer(def);
  }
}

typedef Strategy* Player;
typedef const Strategy* Table[4];

void runGame(const Strategy** players) {
  std::string asHex = asHexString(gDeal);
  printf("%s\n", asHex.c_str());
  Deal deck(gDeal);
  deck.printDeal();

  GameState state(deck);
  bool shotTheMoon;
  float finalScores[4] = {0, 0, 0, 0};
  state.PlayGame(players, finalScores, shotTheMoon, gAnnotator);

  if (shotTheMoon) {
    printf("Shot the moon!\n");
  }
  printf("Final scores: %3.1f %3.1f %3.1f %3.1f\n", finalScores[0], finalScores[1], finalScores[2], finalScores[3]);
}

int main(int argc, char** argv)
{
  parseArgs(argc, argv);

  const Strategy* players[] = { gChampion, gOpponent, gOpponent, gOpponent };
  runGame(players);

  return 0;
}
