#include "lib/VoidBits.h"
#include <algorithm>

VoidBits::~VoidBits()
{}

VoidBits::VoidBits(uint16_t bits)
: mBits(bits)
{}

VoidBits::VoidBits(const VoidBits& other)
: mBits(other.mBits)
{}

PriorityList VoidBits::MakePriorityList(int currentPlayer, const CardDeck& unknownCardsRemaining) const
{
  VoidBits others = ForOthers(currentPlayer);
  PriorityList prioList;
  for (Suit suit=0; suit<4; ++suit) {
    if (unknownCardsRemaining.CountCardsWithSuit(suit) == 0) {
      prioList.push_back(SuitVoids(suit, 3));
    } else {
      uint8_t numVoids = others.CountVoidInSuit(suit);
      prioList.push_back(SuitVoids(suit, numVoids));
    }
  }

  std::sort(prioList.begin(), prioList.end(), SuitVoids::compare);
  return prioList;
}

uint8_t VoidBits::CountVoidInSuit(Suit suit) const
{
  uint8_t count = 0;
  for (int p=0; p<4; ++p) {
    if (isVoid(p, suit))
      ++count;
  }
  return count;
}

void VoidBits::VerifyVoids(const CardHands& hands) const {
#ifndef NDEBUG
  for (int p=0; p<4; ++p) {
    const CardHand& hand = hands[p];
    for (int suit=0; suit<4; ++suit) {
      if (isVoid(p, suit)) {
        assert(hand.CountCardsWithSuit(suit) == 0);
      }
    }
  }
#endif
}
