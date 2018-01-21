#pragma once

#include "lib/PossibilityAnalyzer.h"

class Distribution;

class NoVoidsAnalyzer : public PossibilityAnalyzer {
public:
  NoVoidsAnalyzer(const CardDeck& remaining, const CardHands& hands, const VoidBits& voidBits);
  virtual ~NoVoidsAnalyzer();

  virtual uint128_t Possibilities() const;

  virtual void ActualizePossibility(uint128_t possibility_index, CardHands& hands) const;

  virtual void AddStage(const CardDeck& other_remaining, PriorityList& list);

  virtual void RenderDot(std::ostream& stream) const;

  virtual void ExpectedDistribution(Distribution& distribution, CardHands& hands);

private:
  CardDeck mRemaining;
  CardHands mHands;
  VoidBits mVoidBits;
};
