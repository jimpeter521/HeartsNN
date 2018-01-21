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

const int kBatchSize = 500;

tensorflow::SavedModelBundle gModel;

void loadModel() {
  // This is a hack to allow just in time loading of the model, and loading it only once.
  // TODO: Add command line handling and use the approach in tournament.cpp to load a model
  // only when needed.
  static bool gLoaded = false;
  if (gLoaded)
    return;

  using namespace tensorflow;
  SessionOptions session_options;
  RunOptions run_options;
  auto status = LoadSavedModel(session_options, run_options, "./savedmodel", {kSavedModelTagServe}, &gModel);
  if (!status.ok()) {
     std::cerr << "Failed: " << status;
     exit(1);
  }
  gLoaded = true;
}

void run(int iterations) {
  const double startTime = now();
  const Strategy* players[4];

  Strategy* player;
  Strategy* opponent;

  // TODO: Add command line parsing!
  if (false) {
    loadModel();
    opponent = new SimpleMonteCarlo(false);
    player = new DnnMonteCarlo(gModel, true);
  } else {
    opponent = new RandomStrategy();
    player = new SimpleMonteCarlo(true);
  }

  float totalChampScore = 0;

  for (int i=0; i<iterations; i++)
  {
    // It doesn't matter too much if we randomize seating because the two of clubs will still be dealt to a random
    // seat at the table. But we randomize here to flush out any bugs in the logic for how the knowable state
    // is serialized.
    players[0] = players[1] = players[2] = players[3] = opponent;
    int p = RandomGenerator::gRandomGenerator.range64(4);
    players[p] = player;

    GameState state;
    float finalScores[4] = {0, 0, 0, 0};
    bool shotTheMoon;
    state.PlayGame(players, finalScores, shotTheMoon);
    totalChampScore += finalScores[p];
  }

  printf("Average champion score: %3.1f\n", totalChampScore / iterations);

  delete player;
  delete opponent;

  const double elapsed = now() - startTime;
  const double avgTimePerRun = elapsed / iterations;
  printf("Batch Elapsed time: %4.2f, Avg per iteration: %4.3f\n", elapsed, avgTimePerRun);
}

int main(int argc, char** argv)
{
  const int kTotalIterations = argc==2 ? atoi(argv[1]) : 2;
  assert(kTotalIterations > 0);

  int remainingIterations = kTotalIterations;

  const double startTime = now();
  int doneSoFar = 0;
  while (remainingIterations > 0)
  {
    int iterationsThisBatch = remainingIterations > kBatchSize ? kBatchSize : remainingIterations;
    run(iterationsThisBatch);
    remainingIterations -= iterationsThisBatch;
    doneSoFar += iterationsThisBatch;

    const double elapsed = now() - startTime;
    const double avgTimePerRun = elapsed / doneSoFar;
    const double estimateRemaining = remainingIterations * avgTimePerRun;

    printf("Total Elapsed time: %4.2f, Avg per iteration: %4.3f\n", elapsed, avgTimePerRun);
    printf("Estimated time remaining: %4.2f %4.2fh\n\n", estimateRemaining, estimateRemaining/3600.0);
  }

  return 0;
}
