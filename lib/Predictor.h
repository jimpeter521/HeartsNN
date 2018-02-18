// Predictor.h

#pragma once

#include "lib/Semaphore.h"

#include <tensorflow/cc/saved_model/loader.h>
#include "dlib/threads.h"
#include <forward_list>

namespace tensorflow {
  struct SavedModelBundle;
};

class Predictor
{
public:
  virtual ~Predictor();
  Predictor() {}
  virtual void Predict(const tensorflow::Tensor& mainData, std::vector<tensorflow::Tensor>& outputs) const = 0;
};

class SynchronousPredictor : public Predictor
{
public:
  virtual ~SynchronousPredictor();

  SynchronousPredictor(const tensorflow::SavedModelBundle& model, const std::vector<std::string> output_tensor_names = {});

  virtual void Predict(const tensorflow::Tensor& mainData, std::vector<tensorflow::Tensor>& outputs) const;

protected:
  const tensorflow::SavedModelBundle& mModel;
  const std::vector<std::string> mOutTensorNames;
};

class PooledPredictor : public Predictor
{
public:
  virtual ~PooledPredictor();

  PooledPredictor(const tensorflow::SavedModelBundle& model, const std::vector<std::string> output_tensor_names = {});

  virtual void Predict(const tensorflow::Tensor& mainData, std::vector<tensorflow::Tensor>& outputs) const;

private:
  void EnqueueOneRequest(const tensorflow::Tensor& mainData, std::vector<tensorflow::Tensor>& output, Semaphore& sem) const;

  void ProcessRequests();
  void ProcessOneBatch();

  struct PredictElement {
    PredictElement(const tensorflow::Tensor& mainData
                 , std::vector<tensorflow::Tensor>& output
                 , Semaphore& sem)
    : mMainData(mainData), mOutput(output), mSemaphore(sem)
    {}

    const tensorflow::Tensor& mMainData;
    std::vector<tensorflow::Tensor>& mOutput;
    Semaphore& mSemaphore;
  };

private:
  SynchronousPredictor* mImplPredictor;
  mutable dlib::mutex mQueueMutex;
  mutable std::forward_list<PredictElement> mQueue;
  mutable Semaphore mRequestsPending;
  volatile bool mRunning;
  uint64_t mTaskId;
};
