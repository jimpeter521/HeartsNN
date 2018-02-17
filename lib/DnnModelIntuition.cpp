// lib/DnnModelIntuition.cpp

#include "lib/Card.h"
#include "lib/KnowableState.h"
#include "lib/random.h"
#include "lib/DnnModelIntuition.h"
#include "lib/PossibilityAnalyzer.h"
#include "lib/timer.h"

#include <tensorflow/core/public/session.h>
#include <tensorflow/core/protobuf/meta_graph.pb.h>
#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>
#include <tensorflow/cc/framework/ops.h>

using namespace std;
using namespace tensorflow;

DnnModelIntuition::~DnnModelIntuition() {}

DnnModelIntuition::DnnModelIntuition(const tensorflow::SavedModelBundle& model)
: mPredictor(new SynchronousPredictor(model))
{
}

Card DnnModelIntuition::choosePlay(const KnowableState& state) const
{
  tensorflow::Tensor mainData = state.Transform();

  std::vector<tensorflow::Tensor> outputs;
  mPredictor->Predict(mainData, outputs);

  float playExpectedValue[13];
  return state.ParsePrediction(outputs, playExpectedValue);
}
