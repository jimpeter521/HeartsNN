#include "lib/RandomStrategy.h"
#include "lib/SimpleMonteCarlo.h"
#include "lib/DnnModelStrategy.h"
#include "lib/DnnMonteCarlo.h"
#include "lib/GameState.h"
#include "lib/Deal.h"

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

void run(uint128_t dealIndex, const Strategy* player, const Strategy* opponent) {
  const Strategy* players[4];

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

  const Strategy* player = 0;
  const Strategy* opponent = 0;
  if (argc > 2) {
    loadModel(argv[2]);
    opponent = new SimpleMonteCarlo(false);
    player = new DnnMonteCarlo(gModel, true);
  } else {
    opponent = new RandomStrategy();
    player = new SimpleMonteCarlo(true);
  }

  run(dealIndex, player, opponent);

  delete player;
  delete opponent;

  return 0;
}
