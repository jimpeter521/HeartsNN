#pragma once

#include "lib/Card.h"
#include "lib/CardArray.h"
#include "lib/math.h"
#include "lib/VoidBits.h"
#include "lib/Distribution.h"

#include <ostream>
#include <vector>

class PossibilityAnalyzer {
public:
  PossibilityAnalyzer();
  virtual ~PossibilityAnalyzer();

  virtual uint128_t Possibilities() const = 0;

  virtual void ActualizePossibility(uint128_t possibility_index, CardHands& hands) const = 0;

  virtual void ExpectedDistribution(Distribution& distribution, CardHands& hands) = 0;
    // Compute for each unplayed card the expected number times the player will see the card.
    // This is the probability distribution P(player|card) multipled by Possibilities().

  virtual void AddStage(const CardDeck& other_remaining, PriorityList& list) = 0;

  virtual void RenderDot(std::ostream& stream) const = 0;

  void RenderDotToFile(const std::string& path, const CardDeck& unknownCardsRemaining) const;

  std::string RenderDotToString() const;
};

PossibilityAnalyzer* BuildAnalyzer(unsigned player, const VoidBits& voidBits, PriorityList& priorityList
                                  , const CardDeck& remaining, const CardHands& hands);

class Impossible : public PossibilityAnalyzer {
public:
  Impossible();
  virtual ~Impossible();

  virtual uint128_t Possibilities() const;

  virtual void ActualizePossibility(uint128_t possibility_index, CardHands& hands) const;

  virtual void AddStage(const CardDeck& other_remaining, PriorityList& list);

  virtual void RenderDot(std::ostream& stream) const;

  virtual void ExpectedDistribution(Distribution& distribution, CardHands& hands);
};

class NoneRemaining : public PossibilityAnalyzer {
public:
  NoneRemaining();
  virtual ~NoneRemaining();

  virtual uint128_t Possibilities() const;

  virtual void ActualizePossibility(uint128_t possibility_index, CardHands& hands) const;

  virtual void AddStage(const CardDeck& other_remaining, PriorityList& list);

  virtual void RenderDot(std::ostream& stream) const;

  virtual void ExpectedDistribution(Distribution& distribution, CardHands& hands) {}
};
