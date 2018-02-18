// Predictor.cpp

#include "lib/Predictor.h"
#include "lib/KnowableState.h"
#include <dlib/logger.h>
#include <unistd.h>

using namespace std;
using namespace dlib;
using namespace tensorflow;

static logger dlog("predictor");

namespace {
  vector<string> out_tensor_names(const vector<string>& output_tensor_names) {
    if (output_tensor_names.size() == 0) {
      return vector<string>({"expected_score/expected_score:0", "moon_prob/moon_prob:0"});
    } else {
      return output_tensor_names;
    }
  }
}

// --- Predictor ---

Predictor::~Predictor()
{}

// --- SynchronousPredictor ---

SynchronousPredictor::~SynchronousPredictor()
{}

SynchronousPredictor::SynchronousPredictor(const SavedModelBundle& model, const vector<string> output_tensor_names)
: Predictor()
, mModel(model)
, mOutTensorNames(out_tensor_names(output_tensor_names))
{
}

void SynchronousPredictor::Predict(const Tensor& mainData, vector<Tensor>& outputs) const
{
  auto result = mModel.session->Run({{"main_data:0", mainData}}, mOutTensorNames, {}, &outputs);
  if (!result.ok()) {
    printf("Tensorflow prediction failed: %s\n", result.error_message().c_str());
    exit(1);
  }
}

// --- PooledPredictor ---

static void CopyToDestRow(Eigen::TensorMap<Eigen::Tensor<const float, 2, 1, long>, 16, Eigen::MakePointer> src
                  ,Eigen::TensorMap<Eigen::Tensor<float, 2, 1, long>, 16, Eigen::MakePointer> dst
                  ,int row)
{
  for (int i=0; i<KnowableState::kNumFeatures; ++i) {
    dst(row, i) = src(0, i);
  }
}

static void CopySrcRowToDest(const vector<Tensor>& src, vector<Tensor>& dst, int row) {
  {
    const unsigned kStride = 52;
    const unsigned kOffset = kStride * row;
    auto s = src[0].flat<float>();
    auto d = dst[0].flat<float>();
    for (unsigned i=0; i<kStride; ++i) {
      d(i) = s(kOffset+i);
    }
  }
  {
    const unsigned kStride = 52 * 5;
    const unsigned kOffset = kStride * row;
    auto s = src[1].flat<float>();
    auto d = dst[1].flat<float>();
    for (unsigned i=0; i<kStride; ++i) {
      d(i) = s(kOffset+i);
    }
  }
}

PooledPredictor::~PooledPredictor()
{
  printf("Deleting PooledPredictor\n");
  mRunning = false;
  default_thread_pool().wait_for_task(mTaskId);
}

PooledPredictor::PooledPredictor(const SavedModelBundle& model, const vector<string> output_tensor_names)
: mImplPredictor(new SynchronousPredictor(model, output_tensor_names))
, mQueueMutex()
, mQueue()
, mRequestsPending()
, mRunning(true)
, mTaskId(default_thread_pool().add_task(*this, &PooledPredictor::ProcessRequests))
{
  assert(false);
  dlog.set_level(LALL);
}

void PooledPredictor::ProcessOneBatch()
{
  unsigned numRequests = mRequestsPending.WaitFor(4, 1);
  if (numRequests == 0) {
    return;
  }

  // dlog << LINFO << "Batch: " << numRequests;

  auto_mutex locker(mQueueMutex);

  assert(numRequests > 0);
  assert(!mQueue.empty());

  // There is a race here. Between the call to Drain() and acquiring the lock
  // another request might have been enqueued.
  // But we have acuqired the mutex now, so no more requests can be added
  // until after we exit this function. So, it should be entirely safe to
  // acquire the additional requests.

  std::forward_list<PredictElement> queue; // an empty new queue
  assert(queue.empty());
  mQueue.swap(queue);
  assert(mQueue.empty());

  // We now have all of the pending requests in our local forward_list `queue`.
  unsigned pending = 0;
  for (auto it = queue.begin(); it != queue.end(); ++it)
    ++pending;

  assert(pending >= numRequests);

  if (pending > numRequests) {
    unsigned actual = mRequestsPending.TryDrain();
    assert(pending-numRequests == actual);
    numRequests += actual;
  }

  // dlog << LINFO << "Queue requests: " << numRequests;

  Tensor mainData(DT_FLOAT, TensorShape({numRequests, KnowableState::kNumFeatures}));
  auto matrix = mainData.matrix<float>();

  int row = 0;
  for (auto it = queue.begin(); it != queue.end(); ++it, ++row) {
    PredictElement elem = *it;
    auto src = elem.mMainData.matrix<float>();
    CopyToDestRow(src, matrix, row);
  }

  std::vector<Tensor> outputs;
  mImplPredictor->Predict(mainData, outputs);

  // We have 2 heads for nows: expected_score and moon_prob
  assert(outputs.size() == 2);
  assert(outputs[0].dim_size(0) == numRequests);
  assert(outputs[1].dim_size(0) == numRequests);

  assert(outputs[0].dim_size(1) == kCardsPerDeck);
  assert(outputs[1].dim_size(1) == kCardsPerDeck);
  assert(outputs[1].dim_size(2) == 5);  // moon probs has 5 classes

  row = 0;
  for (auto it = queue.begin(); it != queue.end(); ++it, ++row) {
    PredictElement elem = *it;

    // We start with an empty vector
    assert(elem.mOutput.size() == 0);
    Tensor expected_score(DT_FLOAT, TensorShape({1, 52}));
    Tensor moon_prob(DT_FLOAT, TensorShape({1, 52, 5}));
    elem.mOutput.push_back(expected_score);
    elem.mOutput.push_back(moon_prob);

    CopySrcRowToDest(outputs, elem.mOutput, row);
    elem.mSemaphore.Release();
  }
}

void PooledPredictor::ProcessRequests()
{
  while (mRunning) {
    ProcessOneBatch();
  }
}

void PooledPredictor::Predict(const Tensor& mainData, vector<Tensor>& output) const
{
  static thread_specific_data<Semaphore> doneSemaphore;
  assert(output.size() == 0);
  EnqueueOneRequest(mainData, output, doneSemaphore.data());
  doneSemaphore.data().Acquire();
  assert(output.size() == 2);
  assert(output[0].dim_size(0) == 1);
  assert(output[1].dim_size(0) == 1);

  assert(output[0].dim_size(1) == kCardsPerDeck);
  assert(output[1].dim_size(1) == kCardsPerDeck);
  assert(output[1].dim_size(2) == 5);  // moon probs has 5 classes
}

void PooledPredictor::EnqueueOneRequest(const tensorflow::Tensor& mainData
                                      , std::vector<tensorflow::Tensor>& output
                                      , Semaphore& sem) const
{
  PredictElement elem(mainData, output, sem);
  auto_mutex locker(mQueueMutex);
  mQueue.push_front(elem);
  mRequestsPending.Release();
}
