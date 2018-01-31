#include <stdio.h>
#include <ftw.h>
#include <unistd.h>

#include "lib/RandomStrategy.h"
#include "lib/MonteCarlo.h"
#include "lib/DnnModelIntuition.h"
#include "lib/GameState.h"
#include "lib/Deal.h"
#include "lib/WriteDataAnnotator.h"

#include "lib/random.h"
#include "lib/math.h"
#include "lib/timer.h"

#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>

tensorflow::SavedModelBundle gModel;

void loadModel(const char* modelDirPath) {
  using namespace tensorflow;
  SessionOptions session_options;
  RunOptions run_options;
  auto status = LoadSavedModel(session_options, run_options, modelDirPath, {kSavedModelTagServe}, &gModel);
  if (!status.ok()) {
     std::cerr << "Failed: " << status;
     exit(1);
  }
}

void run(uint128_t dealIndex, StrategyPtr player, StrategyPtr opponent) {
  StrategyPtr players[4];

  for (int i=0; i<4; i++)
  {
    players[0] = players[1] = players[2] = players[3] = opponent;
    players[i] = player;

    Deal deal(dealIndex);
    GameState state(deal);

    float finalScores[4] = {0, 0, 0, 0};
    bool shotTheMoon;
    state.PlayGame(players, finalScores, shotTheMoon);
  }
}

void usage() {
  fprintf(stderr, "Usage: validate <hexDealIndex> [<modelDir>]\n");
  exit(1);
}

int main(int argc, char** argv)
{
  if (argc <= 1) {
    usage();
  }
  const uint128_t dealIndex = parseHex128(argv[1]);

  AnnotatorPtr annotator(new WriteDataAnnotator(true));

  StrategyPtr player;
  StrategyPtr opponent;
  if (argc > 2) {
    loadModel(argv[2]);
    StrategyPtr intuition(new DnnModelIntuition(gModel));
    opponent = intuition;
    player = StrategyPtr(new MonteCarlo(intuition, annotator));
  } else {
    StrategyPtr intuition(new RandomStrategy());
    opponent = intuition;
    player = StrategyPtr(new MonteCarlo(intuition, annotator));
  }

  run(dealIndex, player, opponent);

  return 0;
}
