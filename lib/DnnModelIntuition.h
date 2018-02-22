#pragma once

#include "lib/Strategy.h"
#include "lib/CardArray.h"
#include "lib/Predictor.h"

#include <tensorflow/cc/saved_model/loader.h>

namespace tensorflow {
  struct SavedModelBundle;
};

class DnnModelIntuition : public Strategy
{
public:
  virtual ~DnnModelIntuition();

  DnnModelIntuition(const tensorflow::SavedModelBundle& model, bool pooled=false);

  virtual Card choosePlay(const KnowableState& state, const RandomGenerator& rng) const;

private:
  Predictor* mPredictor;
};
