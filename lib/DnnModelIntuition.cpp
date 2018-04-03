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

DnnModelIntuition::~DnnModelIntuition() {
  delete mPredictor;
}

DnnModelIntuition::DnnModelIntuition(const tensorflow::SavedModelBundle& model, bool pooled)
: mPredictor(0)
{
  if (pooled)
    mPredictor = new PooledPredictor(model);
  else
    mPredictor = new SynchronousPredictor(model);
}

Card DnnModelIntuition::choosePlay(const KnowableState& state, const RandomGenerator& rng) const
{
  FloatMatrix matrix = state.AsFloatMatrix();
  Tensor mainData(DT_FLOAT, TensorShape({1, kCardsPerDeck, KnowableState::kNumFeaturesPerCard}));

  const float* srcData = matrix.data();
  float* dstData = mainData.flat<float>().data();

  memcpy(dstData, srcData, kCardsPerDeck*KnowableState::kNumFeaturesPerCard*sizeof(float));

  std::vector<tensorflow::Tensor> outputs;
  mPredictor->Predict(mainData, outputs);

  float playExpectedValue[13];
  return state.ParsePrediction(outputs, playExpectedValue);
}
