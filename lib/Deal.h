#pragma once

#include <set>

#include "lib/math.h"
#include "lib/Card.h"
#include "lib/CardArray.h"

uint128_t PossibleDealUnknownsToHands(const CardDeck& unknowns, const CardHands& hands);
void DealUnknownsToHands(const CardDeck& unknowns, CardHands& hands);
void DealUnknownsToHands(const CardDeck& unknowns, CardHands& hands, uint128_t index);
void ValidateDealUnknowns(const CardDeck& unknowns, const CardHands& hands);

class Deal {
public:
  Deal();
    // Creates a random deal

  Deal(uint128_t index);
    // Creates a deal for the given index.

  uint128_t dealIndex() const { return mDealIndex; }

  void printDeal() const;

  CardHand dealFor(int player) const;
    // player is a number 0..3
    // Returns a vector of the 13 cards dealt to the player

  int startPlayer() const;
    // Return number of player who has the 2 clubs.

  static uint128_t RandomDealIndex();
    // Generate a random bignum in the range [0, 52!/(13!^4))

  Card PeekAt(int p, int c) const { return mHands[p].NthCard(c); }
  Card PeekAt(int i) const { return PeekAt(i/13, i%13); }
    // For unit tests, peak at the card at a given card location

private:
  void DealHands(uint128_t index);
    // Given an unsigned large integer less than 52!/(13!^4),
    // generate a uniquely determined Hearts/Bridge deal.

private:
  const uint128_t mDealIndex;
  CardHands mHands;
};
