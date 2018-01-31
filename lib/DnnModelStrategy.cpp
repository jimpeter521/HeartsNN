// lib/DnnModelStrategy.cpp

#include "lib/Card.h"
#include "lib/KnowableState.h"
#include "lib/random.h"
#include "lib/DnnModelStrategy.h"
#include "lib/PossibilityAnalyzer.h"
#include "lib/timer.h"

#include <tensorflow/core/public/session.h>
#include <tensorflow/core/protobuf/meta_graph.pb.h>
#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>
#include <tensorflow/cc/framework/ops.h>

using namespace std;
using namespace tensorflow;

DnnModelStrategy::~DnnModelStrategy() {}

DnnModelStrategy::DnnModelStrategy(const tensorflow::SavedModelBundle& model)
: mModel(model)
{
}

Card DnnModelStrategy::choosePlay(const KnowableState& state) const
{
  float playExpectedValue[13];
  Card bestCard = state.TransformAndPredict(mModel, playExpectedValue);
  return bestCard;
}
