#pragma once

#include "lib/Strategy.h"
#include "lib/Annotator.h"

class KnowableState;

class MonteCarlo : public Strategy
{
public:
  virtual ~MonteCarlo();

  MonteCarlo(const StrategyPtr& intuition, const AnnotatorPtr& annotator=0, uint64_t maxAlternates=200);

  virtual Card choosePlay(const KnowableState& state) const;

private:
  StrategyPtr mIntuition;
  const uint64_t kMaxAlternates;
};
