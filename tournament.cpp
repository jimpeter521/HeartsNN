#include "lib/RandomStrategy.h"
#include "lib/MonteCarlo.h"
#include "lib/DnnModelIntuition.h"
#include "lib/GameState.h"

#include "lib/random.h"
#include "lib/math.h"
#include "lib/timer.h"

#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>

#include <getopt.h>
#include <string>
#include <algorithm>

std::vector<std::string> split(const std::string& s, char delimiter=' ')
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter))
  {
    tokens.push_back(token);
  }
  return tokens;
}

enum PlayerRole {
  kChampion,
  kOpponent
};

tensorflow::SavedModelBundle gChampionModel;
tensorflow::SavedModelBundle gOpponentModel;

StrategyPtr loadIntuition(const std::string& path, PlayerRole role) {
  if (path == "random") {
    StrategyPtr intuition(new RandomStrategy());
    return intuition;
  } else {
    using namespace tensorflow;
    SessionOptions session_options;
    RunOptions run_options;
    tensorflow::SavedModelBundle* model = role==kChampion ? &gChampionModel : &gOpponentModel;
    auto status = LoadSavedModel(session_options, run_options, path, {kSavedModelTagServe}, model);
    if (!status.ok()) {
       std::cerr << "Failed: " << status;
       exit(1);
    }
    StrategyPtr intuition(new DnnModelIntuition(*model));
    return intuition;
  }
}

int gNumMatches = 0;
uint128_t* gDeals = 0;

const char* gChampionStr = "random#100";
const char* gOpponentStr = "random";

StrategyPtr gOpponent;
StrategyPtr gChampion;

bool gSaveMoonDeals = true;
bool gQuiet = false;

RandomGenerator rng;

AnnotatorPtr kNoAnnotator(0);


const char* PlayerName(PlayerRole role) {
  return role==kChampion ? "Champion" : "Opponent";
}

StrategyPtr makePlayer(PlayerRole role, const std::string& intuitionName, int rollouts)
{
  StrategyPtr intuition = loadIntuition(intuitionName, role);
  if (rollouts == 0) {
    fprintf(stderr, "Player %s using intuition with %s\n", PlayerName(role), intuitionName.c_str());
    return intuition;
  } else {
    fprintf(stderr, "Player %s using montecarlo on %s with %d rollouts\n", PlayerName(role), intuitionName.c_str(), rollouts);
    const bool kParallel = true;
    return StrategyPtr(new MonteCarlo(intuition, rollouts, kParallel, kNoAnnotator));
  }
}

StrategyPtr makePlayer(PlayerRole role, const std::string& arg)
{
  const int kDefaultRollouts = 40;

  std::string intuitionName;
  int rollouts;

  const char kSep = '#';
  if (arg[arg.size()-1] == kSep) {
    intuitionName = arg.substr(0, arg.size()-1);
    rollouts = kDefaultRollouts;
  } else {
    std::vector<std::string> parts = split(arg, '#');
    assert(parts.size() > 0);
    assert(parts.size() <= 2);

    intuitionName = parts[0];

    if (parts.size() == 1) {
      rollouts = 0;
    } else {
      rollouts = std::stoi(parts[1]);
    }
  }
  return makePlayer(role, intuitionName, rollouts);
}

void usage() {
  const char* lines[] = {
    "Usage: tournament [options...]",
    "  Options:",
    "    -g,--games <int>           the number of games to play (default:1)",
    "    -m,--model <modelPath>     a path to the model to load for `intuion` and `dnnmonte` strategies (default:savemodel)",
    "    -o,--opponent <strategy>   the strategy to use for the `opponent` (default:random)",
    "    -c,--champion <strategy>   the strategy to use for the `champion` (default: simple)",
    "    -d,--deals <dealIndexFile> a file containing deal indexes to play from (default: choose deals at random)",
    "    -h,--help                  print this message",
    0
  };
  for (int i=0; lines[i]!=0; ++i)
    printf("%s\n", lines[i]);
  exit(0);
}

const void randomDeals(int n) {
  gNumMatches = n;
  gDeals = new uint128_t[gNumMatches];
  for (int i=0; i<gNumMatches; ++i)
    gDeals[i] = Deal::RandomDealIndex();
}

void trim(char* line) {
  int len = strlen(line);
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
    { "quiet",  no_argument, NULL, 'q' },
    { "help", no_argument, NULL, 'h' },
    { NULL,                       0, NULL,  0  }
  };

  while (true) {

    int longindex = 0;
    int ch = getopt_long(argc, argv, "m:g:o:c:d:qh", longopts, &longindex);
    if (ch == -1) {
      break;
    }

    switch (ch) {
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

  if (gDeals == 0) {
    randomDeals(1);
  }
}

typedef StrategyPtr Player;
typedef StrategyPtr Table[4];

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

  void Accumulate(StrategyPtr players[4], const GameOutcome& outcome) {
    for (int j=0; j<4; ++j) {
      StrategyPtr player = players[j];
      int playerIndex = player == gChampion ? 0 : 1;
      mPlayer[playerIndex] += outcome.ZeroMeanStandardScore(j);
      mPosition[j] += outcome.ZeroMeanStandardScore(j);
      mCross[playerIndex][j] += outcome.ZeroMeanStandardScore(j);
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

void runOneGame(uint128_t dealIndex, StrategyPtr players[4], Scores& scores, bool& moon) {
  Deal deck(dealIndex);
  GameState state(deck);
  GameOutcome outcome = state.PlayGame(players, rng);
  moon = outcome.shotTheMoon();

  const char* name[2] = {"c", "o"};

  scores.Accumulate(players, outcome);

  if (!gQuiet) {
    for (int i=0; i<4; ++i) {
      StrategyPtr player = players[i];
      int playerIndex = player == gChampion ? 0 : 1;
      printf("%s=%5.1f ", name[playerIndex], outcome.ZeroMeanStandardScore(i));
    }
    if (moon)
      printf("  Shot the moon!\n");
    else
      printf("\n");
  }
}

void runOneMatch(StrategyPtr champion, StrategyPtr opponent, const uint128_t dealIndex, float playerScores[2]) {

  // A match is six games with the same deal of cards to the four positions (N, E, S, W)
  // The two player strategies each occupy two of the table positions.
  // There are six unique arrangements of the two player strategies.
  // We'll rollup all of the game scores into one score for each strategy.
  Table match[6] = {
    { champion, champion, opponent, opponent },
    { champion, opponent, champion, opponent },
    { champion, opponent, opponent, champion },
    { opponent, opponent, champion, champion },
    { opponent, champion, opponent, champion },
    { opponent, champion, champion, opponent },
  };

  Scores matchScores;

  int shotMoon = 0;

  Deal deck(dealIndex);
  if (!gQuiet) {
    std::string asHex = asHexString(dealIndex);
    printf("%s\n", asHex.c_str());
    deck.printDeal();
  }

  for (int i=0; i<6; ++i) {
    bool moon;
    runOneGame(dealIndex, match[i], matchScores, moon);
    if (moon)
      ++shotMoon;
  }

  if (!gQuiet) {
    matchScores.Summarize();
  }

  for (int p=0; p<2; ++p)
    playerScores[p] += matchScores.mPlayer[p];

  if (gSaveMoonDeals && shotMoon > 1) {
    std::string hex = asHexString(dealIndex);
    FILE* f = fopen("moonhands.txt", "a");
    fprintf(f, "%s\n", hex.c_str());
    fclose(f);
  }

}

float runOneTournament(StrategyPtr champion, StrategyPtr opponent) {
  float playerScores[2] = {0};

  for (int i=0; i<gNumMatches; ++i) {
    runOneMatch(champion, opponent, gDeals[i], playerScores);
  }

  if (!gQuiet) {
    printf("Champion: %4.2f\n", playerScores[0] / (gNumMatches*6.0));
    printf("Opponent: %4.2f\n", playerScores[1] / (gNumMatches*6.0));
    if (playerScores[0] <= playerScores[1]) {
      printf("Champion wins\n");
    } else {
      printf("Oppponent wins\n");
    }
  }

  return playerScores[0] / (gNumMatches*6.0) ; // the champ's score
}

#if 1
int main(int argc, char** argv)
{
  parseArgs(argc, argv);

  gChampion = makePlayer(kChampion, gChampionStr);
  gOpponent = makePlayer(kOpponent, gOpponentStr);

  runOneTournament(gChampion, gOpponent);

  return 0;
}
#else
int main(int argc, char** argv)
{
  const char* modelPath = "pereubu.save/d2w200_relu";
  gQuiet = true;
  gNumMatches = 50;
  randomDeals(gNumMatches);

  for (int rollouts=1; rollouts<=51; rollouts+=10) {
    // Champion is pure intuition
    gChampion = makePlayer(kChampion,  "random", 100);

    // Opponent is monte carlo on random intuition with given number of rollouts
    gOpponent = makePlayer(kOpponent, modelPath, rollouts);

    float champScore = runOneTournament(gChampion, gOpponent);
    printf("%d\t%f\n", rollouts, champScore);
    fflush(stdout);
  }
  return 0;
}
#endif
