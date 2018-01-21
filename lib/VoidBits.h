#pragma once

#include "lib/Card.h"
#include "lib/CardArray.h"
#include <vector>

struct SuitVoids
{
  SuitVoids(Suit s, uint8_t n): suit(s), numVoids(n) { assert(n<=3); }
  SuitVoids(): suit(kUnknown), numVoids(0) {}
  Suit    suit;
  uint8_t numVoids;

  // sorts in ascending order by numVoids
  // we will process in descending order, but using .pop_back()
  static bool compare(const SuitVoids& s1, const SuitVoids& s2) { return s1.numVoids < s2.numVoids; }
};

typedef std::vector<SuitVoids> PriorityList;

class VoidBits
{
public:
  ~VoidBits();

  VoidBits(uint16_t bits=0);

  VoidBits(const VoidBits& other);

  void operator=(const VoidBits& other) { mBits = other.mBits; }

  VoidBits ForOthers(int currentPlayer) const { return VoidBits(OthersKnownVoid(currentPlayer)); }

  PriorityList MakePriorityList(int currentPlayer, const CardDeck& unknownCardsRemaining) const;

  bool isVoid(int player, Suit suit) const
  {
    return (mBits & VoidBit(player, suit)) != 0;
  }

  void setIsVoid(int player, Suit suit)
  {
    mBits |= VoidBit(player, suit);
  }

  // bool areAnyPlayersKnownVoid() const { return mBits != 0; }

  uint8_t CountVoidInSuit(Suit suit) const;

  void VerifyVoids(const CardHands& hands) const;

private:
  static inline int VoidBit(int player, Suit suit)
  {
    // This scheme creates one nibble per suit, with each nibble containing one bit per player.
    return 1u << (4*suit + player);
  }

  unsigned OthersKnownVoid(int currentPlayer) const
  {
    // Here we mask out the bits for the current player. Since there is one nibble per Suit
    // we have to touch each of the four nibbles. The mask will be one of the values 0x1111, 0x2222, 0x4444, 0x8888.
    const unsigned mask = 0x1111 << currentPlayer;
    return mBits & ~mask;
  }

private:
  uint16_t mBits;
};
