#pragma once

#include "lib/Strategy.h"
#include "lib/CardArray.h"

class RandomStrategy : public Strategy
{
public:
  virtual ~RandomStrategy();

  RandomStrategy();

  virtual Card choosePlay(const KnowableState& state) const;

  static const RandomStrategy* const gSingleton;
};
