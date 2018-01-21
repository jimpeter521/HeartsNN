#pragma once

#include "lib/Strategy.h"

class PossibilityAnalyzer;

namespace tensorflow {
  struct SavedModelBundle;
};

class DnnMonteCarlo : public Strategy
{
public:
  virtual ~DnnMonteCarlo();

  DnnMonteCarlo(const tensorflow::SavedModelBundle& model, bool writeData=false);

  virtual Card choosePlay(const KnowableState& state, Annotator* annotator = 0) const;

private:
  const std::string mModelDir;
  const std::string mHash;
  const Strategy* mRolloutStrategy;

  Annotator* mWriteDataAnnotator;
};
