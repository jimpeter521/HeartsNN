#pragma once

#include "lib/Card.h"
#include "lib/CardArray.h"

class Annotator;
class KnowableState;

class Strategy
{
public:
  virtual ~Strategy();
  Strategy();

  virtual Card choosePlay(const KnowableState& state, Annotator* annotator = 0) const = 0;
};
