// Predictor.cpp

#include "lib/Predictor.h"

namespace {
  std::vector<std::string> out_tensor_names(const std::vector<std::string>& output_tensor_names) {
    if (output_tensor_names.size() == 0) {
      return std::vector<std::string>({"expected_score/expected_score:0"});
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

SynchronousPredictor::SynchronousPredictor(const tensorflow::SavedModelBundle& model, const std::vector<std::string> output_tensor_names)
: Predictor()
, mModel(model)
, mOutTensorNames(out_tensor_names(output_tensor_names))
{
}

void SynchronousPredictor::Predict(const tensorflow::Tensor& mainData, std::vector<tensorflow::Tensor>& outputs) const
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

PooledPredictor::PooledPredictor(const tensorflow::SavedModelBundle& model, const std::vector<std::string> output_tensor_names)
: mImplPredictor(new SynchronousPredictor(model, output_tensor_names))
{
}

void PooledPredictor::Predict(const tensorflow::Tensor& mainData, std::vector<tensorflow::Tensor>& outputs) const
{
  return mImplPredictor->Predict(mainData, outputs);
}
