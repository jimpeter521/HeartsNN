#pragma once

#include "lib/PossibilityAnalyzer.h"

struct TwoOpponents
{
  unsigned p[2];
};

class TwoOpponentsGetSuit : public PossibilityAnalyzer
{
public:
  TwoOpponentsGetSuit(unsigned player, const VoidBits& voidBits, Suit suit, const CardDeck& remainingOfSuit
                    , const CardDeck& otherRemaining, const CardHands& hands);
  virtual ~TwoOpponentsGetSuit();

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
  const TwoOpponents mOpponents;

  std::vector<PossibilityAnalyzer*>  mWays;
};

class Ways: public PossibilityAnalyzer
{
public:
  Ways(unsigned player, const VoidBits& voidBits, Suit suit, const CardDeck& remainingOfSuit
      , const CardDeck& otherRemaining, const CardHands& hands, const TwoOpponents& opponents, unsigned numFirstPlayer);

  virtual ~Ways();

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
  const unsigned A; // the first opponent who still may have suit
  const unsigned B; // the second opponent who still may have suit
  const unsigned mNumFirstPlayer;
  PossibilityAnalyzer* mNextStage;
  CardHands mHands;
};
