// Predictor.h

#pragma once

#include <tensorflow/cc/saved_model/loader.h>

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
  SynchronousPredictor* mImplPredictor;
};
