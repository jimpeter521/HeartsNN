#pragma once

#include "lib/Strategy.h"
#include "lib/Annotator.h"

class KnowableState;

class MonteCarlo : public Strategy
{
public:
  virtual ~MonteCarlo();

  MonteCarlo(const StrategyPtr& intuition, uint32_t minAlternates, uint32_t maxAlternates, float timeBudget, const AnnotatorPtr& annotator=0);

  virtual Card choosePlay(const KnowableState& state) const;

private:
  StrategyPtr mIntuition;
  const uint32_t kMinAlternates;
  const uint32_t kMaxAlternates;
  const float kTimeBudget;
};
