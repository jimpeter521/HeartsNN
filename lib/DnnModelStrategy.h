#pragma once

#include "lib/Strategy.h"
#include "lib/CardArray.h"

#include <tensorflow/cc/saved_model/loader.h>

namespace tensorflow {
  struct SavedModelBundle;
};

class DnnModelStrategy : public Strategy
{
public:
  virtual ~DnnModelStrategy();

  DnnModelStrategy(const tensorflow::SavedModelBundle& model);

  virtual Card choosePlay(const KnowableState& state) const;

private:
  const tensorflow::SavedModelBundle& mModel;
};
