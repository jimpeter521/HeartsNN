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

  Predictor(const tensorflow::SavedModelBundle& model, const std::vector<std::string> output_tensor_names = {});

  virtual void Predict(const tensorflow::Tensor& mainData, std::vector<tensorflow::Tensor>& outputs) const = 0;

protected:
  const tensorflow::SavedModelBundle& mModel;
  const std::vector<std::string> mOutTensorNames;
};

class SynchronousPredictor : public Predictor
{
public:
  virtual ~SynchronousPredictor();

  virtual void Predict(const tensorflow::Tensor& mainData, std::vector<tensorflow::Tensor>& outputs) const;
};
