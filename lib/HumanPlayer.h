#pragma once

#include "lib/Strategy.h"

class HumanPlayer : public Strategy {
public:
  virtual ~HumanPlayer();
  HumanPlayer(const AnnotatorPtr &annotator = 0);

  virtual Card choosePlay(const KnowableState &state,
                          const RandomGenerator &rng) const;
};
