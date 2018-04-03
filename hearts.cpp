#include "lib/RandomStrategy.h"
#include "lib/MonteCarlo.h"
#include "lib/DnnModelIntuition.h"
#include "lib/GameState.h"
#include "lib/WriteDataAnnotator.h"
#include "lib/WriteTrainingDataSets.h"

#include "lib/random.h"
#include "lib/math.h"
#include "lib/timer.h"

#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>
#include <sys/stat.h>

const int kConcurrency = 2 + (std::thread::hardware_concurrency()/2);
const int kIterationsPerTask = 100;
const int kBatchSize = kConcurrency*kIterationsPerTask;
dlib::thread_pool tp(kConcurrency);

const char* gModelPath = 0;
tensorflow::SavedModelBundle gModel;

static bool gLoaded = false;
dlib::mutex gLoadModelMutex;

void loadModel() {
  // This is a hack to allow just in time loading of the model, and loading it only once.
  // TODO: Add command line handling and use the approach in tournament.cpp to load a model
  // only when needed.
  dlib::auto_mutex lock(gLoadModelMutex);
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
  std::cout << "Model loaded" << std::endl;
  gLoaded = true;
}

StrategyPtr getIntuition() {
  if (gModelPath) {
    loadModel();
    return StrategyPtr(new DnnModelIntuition(gModel, false));
  } else {
    return StrategyPtr(new RandomStrategy());
  }
}

float run_iterations_task(int kIterationsPerTask, StrategyPtr opponent)
{
  const RandomGenerator& rng = RandomGenerator::ThreadSpecific();
  const uint32_t kNumAlternates = gModelPath ? 100 : 5000;

  // The `player` uses monte carlo and will generate data
  // AnnotatorPtr annotator(new WriteDataAnnotator());
  AnnotatorPtr annotator(new WriteTrainingDataSets());
  StrategyPtr player(new MonteCarlo(opponent, kNumAlternates, false, annotator));

  StrategyPtr players[4];

  float totalChampScore = 0.0;
  for (int i=0; i<kIterationsPerTask; ++i)
  {
    // It doesn't matter too much if we randomize seating because the two of clubs will still be dealt to a random
    // seat at the table. But we randomize here to flush out any bugs in the logic for how the knowable state
    // is serialized -- we don't want the current player to always be player 0.
    int p = i % 4;
    players[0] = players[1] = players[2] = players[3] = opponent;
    players[p] = player;

    GameState state;
    GameOutcome outcome = state.PlayGame(players, rng);
    totalChampScore += outcome.ZeroMeanStandardScore(p);
  }

  return totalChampScore;
}

void run(int iterations, const StrategyPtr& opponent)
{
  assert((iterations % kConcurrency) == 0);
  const double startTime = now();

  std::vector<std::future<float>> totals(kConcurrency);

  const int perTask = iterations / kConcurrency;
  assert(perTask >= 1);
  for (int i=0; i<kConcurrency; i++)
    totals[i] = dlib::async(tp, [perTask, opponent]() {return run_iterations_task(perTask, opponent);});

  float totalChampScore = 0;
  for (int i=0; i<kConcurrency; i++)
    totalChampScore += totals[i].get();

  printf("Average champion score: %3.1f\n", totalChampScore / iterations);

  const double elapsed = now() - startTime;
  const double avgTimePerRun = elapsed / iterations;
  printf("Batch Elapsed time: %4.2f, Avg per iteration: %4.3f\n", elapsed, avgTimePerRun);
}

int roundUpToConcurrency(int i)
{
  assert(i > 0);
  const int perTask = (i+kConcurrency-1) / kConcurrency;
  assert(perTask > 0);
  return perTask * kConcurrency;
}

int main(int argc, char** argv)
{
  const int kTotalIterations = argc>=2 ? roundUpToConcurrency(atoi(argv[1])) : kConcurrency;
  assert(kTotalIterations >= kConcurrency);
  assert((kTotalIterations % kConcurrency) == 0);

  const bool kUseDNN = argc>=3;
  gModelPath = kUseDNN ? argv[2] : 0;

  int remainingIterations = kTotalIterations;

  // Each of the three opponents will use intuition only and not write data.
  StrategyPtr opponent = getIntuition();

  const char* dataDirPath = "data";
  int err = mkdir(dataDirPath, 0777);
  if (err != 0 && errno != EEXIST) {
    const char* errmsg = strerror(errno);
    fprintf(stderr, "mkdir %s failed: %s\n", dataDirPath, errmsg);
    assert(err != 0);
  }

  const double startTime = now();
  int doneSoFar = 0;
  while (remainingIterations > 0)
  {
    int iterationsThisBatch = remainingIterations > kBatchSize ? kBatchSize : remainingIterations;
    assert((iterationsThisBatch % kConcurrency) == 0);
    run(iterationsThisBatch, opponent);
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
