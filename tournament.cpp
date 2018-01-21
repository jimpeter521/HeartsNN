#include "lib/RandomStrategy.h"
#include "lib/SimpleMonteCarlo.h"
#include "lib/DnnModelStrategy.h"
#include "lib/DnnMonteCarlo.h"
#include "lib/GameState.h"

#include "lib/random.h"
#include "lib/math.h"
#include "lib/timer.h"

#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>

#include <getopt.h>
#include <string>
#include <algorithm>

tensorflow::SavedModelBundle gModel;
void loadModel(const char* path) {
  using namespace tensorflow;
  SessionOptions session_options;
  RunOptions run_options;
  auto status = LoadSavedModel(session_options, run_options, path, {kSavedModelTagServe}, &gModel);
  if (!status.ok()) {
     std::cerr << "Failed: " << status;
     exit(1);
  }
}

int gNumMatches = 0;
uint128_t* gDeals = 0;
Strategy* gOpponent = 0;
Strategy* gChampion = 0;
const char* gModelPath = "./savedmodel";

bool gSaveMoonDeals = true;

Strategy* makePlayer(const char* arg) {
  Strategy* player = 0;
  if (arg == 0 || std::string(arg) == std::string("random")) {
    player = new RandomStrategy();
  }
  else if (std::string(arg) == std::string("simple")) {
    player = new SimpleMonteCarlo();
  }
  else if (std::string(arg) == std::string("intuition")) {
    loadModel(gModelPath);
   player = new DnnModelStrategy(gModel);
  }
  else if (std::string(arg) == std::string("dnnmonte")) {
     loadModel(gModelPath);
    player = new DnnMonteCarlo(gModel);
  }
  else {
    loadModel(arg);
    player = new DnnMonteCarlo(gModel);
  }
  return player;
}

void usage() {
  const char* lines[] = {
    "Usage: tournament [--games <int>] [--model <modelPath>] [--opponent <strategy>]  [--champion <strategy>] [--deals <dealIndexFile>",
    "  <strategy> may be either of the strings ['random', 'simple', 'intuition', 'dnnmonte'] or a path to a saved model directory",
    "  If unspecified, the default <strategy> for opponent is 'random', and for champion is 'simple'",
    "  The intuition and dnnmonte load the model at modelPath which defaults to './savedmodel'",
    "  The default number of games is 1.",
    0
  };
  for (int i=0; lines[i]!=0; ++i)
    printf("%s\n", lines[i]);
  exit(0);
}

const void randomDeals(int n) {
  assert(n>0);
  assert(n<=1000);
  gNumMatches = n;
  gDeals = new uint128_t[gNumMatches];
  for (int i=0; i<gNumMatches; ++i)
    gDeals[i] = Deal::RandomDealIndex();
}

void trim(char* line) {
  int len = strlen(line);
  assert(line[len] == 0);
  while (isspace(line[len-1])) {
    --len;
    line[len] = 0;
  }
}

const void readDeals(const char* path) {
  const int kMaxLines = 1000;
  gDeals = new uint128_t[kMaxLines];
  FILE* f = fopen(path, "r");
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  int i = 0;
  while ((linelen = getline(&line, &linecap, f)) > 0) {
    trim(line);
    assert(i < kMaxLines);
    gDeals[i++] = parseHex128(line);
    printf("Using deal %s -> %s\n", line, asHexString(gDeals[i-1]).c_str());
  }
  gNumMatches = i;
}

void parseArgs(int argc, char** argv) {
  const struct option longopts[] = {
    { "model",  required_argument, NULL, 'm' },
    { "games",  required_argument, NULL, 'g' },
    { "opponent", required_argument, NULL, 'o' },
    { "champion", required_argument, NULL, 'c' },
    { "deals", required_argument, NULL, 'd'},
    { "help", no_argument, NULL, 'h' },
    { NULL,                       0, NULL,  0  }
  };

  while (true) {

    int longindex = 0;
    int ch = getopt_long(argc, argv, "m:g:o:c:d:h", longopts, &longindex);
    if (ch == -1) {
      break;
    }

    switch (ch) {
      case 'm':
      {
         gModelPath = optarg;
         break;
      }
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
      case 'h':
      default:
      {
        usage();
        break;
      }
    }
  }

  if (gDeals == 0) {
    randomDeals(1);
  }

  if (gOpponent == 0) {
    const char* def = "random";
    printf("Setting opponent to %s\n", def);
    gOpponent = makePlayer(def);
  }
  if (gChampion == 0) {
    const char* def = "simple";
    printf("Setting champion to %s\n", def);
    gChampion = makePlayer(def);
  }
}

typedef Strategy* Player;
typedef const Strategy* Table[4];
typedef Table Match[];

// This Scores struct is useful for analyzing the results of one match.
// It accumulates both total scores for each player strategy, but also
// the scores for each position at the table across the six games in the match.
struct Scores {
  float mPlayer[2];
  float mPosition[4];
  float mCross[2][4];

  Scores()
  {
    std::fill(mPlayer, mPlayer+2, 0.0);
    std::fill(mPosition, mPosition+4, 0.0);
    std::fill(&mCross[0][0], &mCross[2][0], 0.0);
  }

  void Accumulate(const Strategy** players, const float scores[4]) {
    for (int j=0; j<4; ++j) {
      const Strategy* player = players[j];
      assert(player==gChampion || player==gOpponent);
      int playerIndex = player == gChampion ? 0 : 1;
      mPlayer[playerIndex] += scores[j];
      mPosition[j] += scores[j];
      mCross[playerIndex][j] += scores[j];
    }
  }

  void Summarize() const {
   const char* name[2] = {"c", "o"};
   printf("\n");
    for (int i=0; i<2; ++i) {
      printf("%s ", name[i]);
      for (int j=0; j<4; ++j) {
        printf("%5.1f ", mCross[i][j] / 3.0);
      }
      printf("| %5.1f\n", mPlayer[i] / 6.0);
    }
    for (int j=0; j<4; ++j) {
      printf("%5.1f ", mPosition[j] / 6.0);
    }
    printf("\n\n");
  }
};

void runOneGame(uint128_t dealIndex, const Strategy** players, Scores& scores, bool& moon) {
  Deal deck(dealIndex);
  GameState state(deck);
  float finalScores[4] = {0, 0, 0, 0};
  state.PlayGame(players, finalScores, moon);

  const char* name[2] = {"c", "o"};

  scores.Accumulate(players, finalScores);

  for (int i=0; i<4; ++i) {
    const Strategy* player = players[i];
    assert(player==gChampion || player==gOpponent);
    int playerIndex = player == gChampion ? 0 : 1;
    printf("%s=%5.1f ", name[playerIndex], finalScores[i]);
  }
  if (moon)
    printf("  Shot the moon!\n");
  else
    printf("\n");
}

void runOneMatch(const uint128_t dealIndex, float playerScores[2]) {

  // A match is six games with the same deal of cards to the four positions (N, E, S, W)
  // The two player strategies each occupy two of the table positions.
  // There are six unique arrangements of the two player strategies.
  // We'll rollup all of the game scores into one score for each strategy.
  Table match[6] = {
    { gChampion, gChampion, gOpponent, gOpponent },
    { gChampion, gOpponent, gChampion, gOpponent },
    { gChampion, gOpponent, gOpponent, gChampion },
    { gOpponent, gOpponent, gChampion, gChampion },
    { gOpponent, gChampion, gOpponent, gChampion },
    { gOpponent, gChampion, gChampion, gOpponent },
  };

  Scores matchScores;

  int shotMoon = 0;

  std::string asHex = asHexString(dealIndex);
  printf("%s\n", asHex.c_str());
  Deal deck(dealIndex);
  deck.printDeal();

  for (int i=0; i<6; ++i) {
    bool moon;
    runOneGame(dealIndex, match[i], matchScores, moon);
    if (moon)
      ++shotMoon;
  }

  matchScores.Summarize();

  for (int p=0; p<2; ++p)
    playerScores[p] += matchScores.mPlayer[p];

  if (gSaveMoonDeals && shotMoon > 1) {
    std::string hex = asHexString(dealIndex);
    FILE* f = fopen("moonhands.txt", "a");
    fprintf(f, "%s\n", hex.c_str());
    fclose(f);
  }

}

int main(int argc, char** argv)
{
  parseArgs(argc, argv);

  float playerScores[2] = {0};

  for (int i=0; i<gNumMatches; ++i) {
    runOneMatch(gDeals[i], playerScores);
  }

  printf("Champion: %4.2f\n", playerScores[0] / (gNumMatches*6.0));
  printf("Opponent: %4.2f\n", playerScores[1] / (gNumMatches*6.0));
  if (playerScores[0] <= playerScores[1]) {
    printf("Champion wins\n");
  } else {
    printf("Oppponent wins\n");
  }

  return 0;
}
