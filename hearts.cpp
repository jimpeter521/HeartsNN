#include "lib/RandomStrategy.h"
#include "lib/MonteCarlo.h"
#include "lib/DnnModelIntuition.h"
#include "lib/GameState.h"
#include "lib/WriteDataAnnotator.h"

#include "lib/random.h"
#include "lib/math.h"
#include "lib/timer.h"

#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>

const int kBatchSize = 500;

const char* gModelPath = 0;
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
  auto status = LoadSavedModel(session_options, run_options, gModelPath, {kSavedModelTagServe}, &gModel);
  if (!status.ok()) {
     std::cerr << "Failed: " << status;
     exit(1);
  }
  gLoaded = true;
}

StrategyPtr getIntuition() {
  if (gModelPath) {
    loadModel();
    return StrategyPtr(new DnnModelIntuition(gModel));
  } else {
    return StrategyPtr(new RandomStrategy());
  }
}

void run(int iterations) {
  const double startTime = now();

  AnnotatorPtr annotator(new WriteDataAnnotator());

  StrategyPtr intuition = getIntuition();

  // The `player` uses monte carlo and will generate data
  const uint32_t kMinAlternates = 30;
  const uint32_t kMaxAlternates = gModelPath!=0 ? 50 : 2000;
  const float kTimeBudget = 0.25;
  StrategyPtr player(new MonteCarlo(intuition, kMinAlternates, kMaxAlternates, kTimeBudget, annotator));

  // Each of the three opponents will use intuition only and not write data.
  StrategyPtr opponent(intuition);

  float totalChampScore = 0;

  RandomGenerator rng;

  StrategyPtr players[4];
  for (int i=0; i<iterations; i++)
  {
    // It doesn't matter too much if we randomize seating because the two of clubs will still be dealt to a random
    // seat at the table. But we randomize here to flush out any bugs in the logic for how the knowable state
    // is serialized -- we don't want the current player to always be player 0.
    players[0] = players[1] = players[2] = players[3] = opponent;
    int p = rng.range64(4);
    players[p] = player;

    GameState state;
    GameOutcome outcome = state.PlayGame(players, rng);
    totalChampScore += outcome.modifiedScore(p);
  }

  printf("Average champion score: %3.1f\n", totalChampScore / iterations);

  const double elapsed = now() - startTime;
  const double avgTimePerRun = elapsed / iterations;
  printf("Batch Elapsed time: %4.2f, Avg per iteration: %4.3f\n", elapsed, avgTimePerRun);
}

int main(int argc, char** argv)
{
  const int kTotalIterations = argc>=2 ? atoi(argv[1]) : 2;
  assert(kTotalIterations > 0);

  const bool kUseDNN = argc>=3;
  gModelPath = kUseDNN ? argv[2] : 0;

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
