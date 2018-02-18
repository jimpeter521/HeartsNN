// Predictor.cpp

#include "lib/Predictor.h"
#include <dlib/logger.h>

using namespace std;
using namespace dlib;
using namespace tensorflow;

logger dlog("predictor");

namespace {
  vector<string> out_tensor_names(const vector<string>& output_tensor_names) {
    if (output_tensor_names.size() == 0) {
      return vector<string>({"expected_score/expected_score:0"});
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

PooledPredictor::~PooledPredictor()
{}

PooledPredictor::PooledPredictor(const SavedModelBundle& model, const vector<string> output_tensor_names)
: mImplPredictor(new SynchronousPredictor(model, output_tensor_names))
, mThreadPool(default_thread_pool())
, mQueueMutex()
, mQueue()
, mRequestsPending()
{
  dlog.set_level(LALL);
  dlog << LINFO << "PooledPredictor start with num threads: " << mThreadPool.num_threads_in_pool();
}

void PooledPredictor::Predict(const Tensor& mainData, vector<Tensor>& output) const
{
  Semaphore doneSemaphore;
  EnqueueOneRequest(mainData, output, doneSemaphore);
  doneSemaphore.Acquire();
  dlog << LINFO << "One request completed";
  assert(output.size() > 0);
}

void PooledPredictor::EnqueueOneRequest(const tensorflow::Tensor& mainData
                                      , std::vector<tensorflow::Tensor>& output
                                      , Semaphore& sem) const
{
  dlog << LINFO << "EnqueueOneRequest";
  PredictElement elem(mainData, output, sem);
  auto_mutex locker(mQueueMutex);
  mQueue.push_front(elem);
  mRequestsPending.Release();
}
