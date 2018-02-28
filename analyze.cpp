#include "lib/RandomStrategy.h"
#include "lib/MonteCarlo.h"
#include "lib/GameState.h"
#include "lib/DnnModelIntuition.h"
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

StrategyPtr gOpponent;
StrategyPtr gChampion;
uint128_t gDeal = kUnset;

using namespace std;
using namespace tensorflow;

tensorflow::SavedModelBundle gModel;

const bool parallel = false;

StrategyPtr makePlayer(const char* arg) {
  StrategyPtr player;
  if (arg == 0 || std::string(arg) == std::string("random")) {
    player = StrategyPtr(new RandomStrategy());
  }
  else if (std::string(arg) == std::string("simple")) {
    StrategyPtr intuition = StrategyPtr(new RandomStrategy());
    AnnotatorPtr annotator(0);
    player = StrategyPtr(new MonteCarlo(intuition, 1000, parallel, annotator));
  }
  else {
    SessionOptions session_options;
    RunOptions run_options;
    auto status = LoadSavedModel(session_options, run_options, arg, {kSavedModelTagServe}, &gModel);
    if (!status.ok()) {
       std::cerr << "Failed: " << status;
       exit(1);
    }
    StrategyPtr intuition = StrategyPtr(new DnnModelIntuition(gModel));
    AnnotatorPtr annotator(new DnnMonteCarloAnnotator(gModel));
    player = StrategyPtr(new MonteCarlo(intuition, 100, parallel, annotator));
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

  if (!gOpponent) {
    const char* def = "simple";
    printf("Setting opponent to %s\n", def);
    gOpponent = makePlayer(def);
  }
  if (!gChampion) {
    const char* def = "savedmodel";
    printf("Setting champion to %s\n", def);
    gChampion = makePlayer(def);
  }
}

typedef StrategyPtr Player;
typedef StrategyPtr Table[4];

void runGame(StrategyPtr players[4]) {
  std::string asHex = asHexString(gDeal);
  printf("%s\n", asHex.c_str());
  Deal deck(gDeal);
  deck.printDeal();

  GameState state(deck);
  GameOutcome outcome = state.PlayGame(players, generator);

  if (outcome.shotTheMoon()) {
    printf("Shot the moon!\n");
  }
  printf("Final scores: %3.1f %3.1f %3.1f %3.1f\n", outcome.standardScore(0), outcome.standardScore(1)
            , outcome.standardScore(2), outcome.standardScore(3));
}

int main(int argc, char** argv)
{
  parseArgs(argc, argv);

  StrategyPtr players[] = { gChampion, gOpponent, gOpponent, gOpponent };
  runGame(players);

  return 0;
}
