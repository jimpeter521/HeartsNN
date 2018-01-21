#pragma once

#include "lib/Strategy.h"

class PossibilityAnalyzer;

class SimpleMonteCarlo : public Strategy
{
public:
  virtual ~SimpleMonteCarlo();

  SimpleMonteCarlo(bool writeData=false);

  virtual Card choosePlay(const KnowableState& state, Annotator* annotator = 0) const;

private:
  const std::string mHash;
  const Strategy* mRolloutStrategy;

  Annotator* mWriteDataAnnotator;
};
