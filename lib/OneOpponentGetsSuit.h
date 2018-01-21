#pragma once

#include "lib/PossibilityAnalyzer.h"

class OneOpponentGetsSuit : public PossibilityAnalyzer {
public:
  OneOpponentGetsSuit(unsigned player, const VoidBits& voidBits, Suit suit, const CardDeck& remainingOfSuit
          , const CardDeck& otherRemaining, const CardHands& hands);
  virtual ~OneOpponentGetsSuit();

  virtual uint128_t Possibilities() const;

  virtual void ActualizePossibility(uint128_t possibility_index, CardHands& hands) const;

  virtual void AddStage(const CardDeck& other_remaining, PriorityList& list);

  virtual void RenderDot(std::ostream& stream) const;

  virtual void ExpectedDistribution(Distribution& distribution, CardHands& hands);

private:
  const unsigned mPlayer;
  const VoidBits mVoidBits;
  const Suit     mSuit;
  const CardDeck mRemainingOfSuit;
  const CardDeck mOtherRemaining;
  const unsigned mOpponent;
  PossibilityAnalyzer* mNextStage;
  CardHands mHands;
};
