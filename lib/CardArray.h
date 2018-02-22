// lib/CardArray.h

#pragma once

#include <assert.h>
#include <stdint.h>
#include <functional>
#include <vector>

#include "lib/Card.h"
#include "lib/random.h"
#include "lib/Bits.h"

enum CardArrayInit {
  kEmpty,
  kFull,
  kGiven
};

class CardArray
{
public:
  typedef unsigned Size_t;

  CardArray(CardArrayInit init=kEmpty, Size_t capacity=kCardsPerHand) : mCapacity(capacity), mCardBits(0) {
    if (init == kFull) {
      assert(capacity==kCardsPerDeck);
      mCardBits = (1L<<52)-1;
    }
  }

  class iterator
  {
  public:
    iterator(const CardArray& arr): mCardBits(arr.mCardBits) { }

    bool done() { return mCardBits == 0; }

    Card next() {
      assert(!done());
      Card card = LeastSetBitIndex(mCardBits);
      mCardBits &= ~(1UL << card);
      return card;
    }

  private:
    uint64_t mCardBits;
  };

  class reverseit
  {
  public:
    reverseit(const CardArray& arr): mCardBits(arr.mCardBits) { }

    bool done() { return mCardBits == 0; }

    Card next() {
      assert(!done());
      Card card = GreatestSetBitIndex(mCardBits);
      mCardBits &= ~(1L << card);
      return card;
    }

  private:
    uint64_t mCardBits;
  };

  CardArray(const CardArray& other): mCapacity(other.mCapacity), mCardBits(other.mCardBits) {
    assert(mCardBits < (1L<<52));
  }

  void operator=(const CardArray& other) {
    mCapacity = other.mCapacity;
    mCardBits = other.mCardBits;
    assert(mCardBits < (1L<<52));
  }

  void Merge(const CardArray& other);
    // Merge the other CardArray into this CardArray.
    // We expect/require that the two CardArrays contain disjoint sets of cards.
    // This array must remain in sorted order

  void Subtract(const CardArray& subset);
    // Remove the subset CardArray from this CardArray.
    // The subset must be a true subset, i.e. not contain any cards not in this CardArray.

  void Init(const Card* cards, Size_t size);

  void Append(Card card) { InsertCard(card); }

  void Insert(Card card) { InsertCard(card); }

  Size_t Size() const { return CountBits(mCardBits); }
    // Size is the number of cards currently in the array (hand or deck).

  Size_t Capacity() const { return mCapacity; }

  void PrepForDeal(Size_t capacity) { mCapacity = capacity; mCardBits = 0;}
  void SetForDeal(const CardArray& other) { *this = other; mCapacity = Size(); }

  Size_t AvailableCapacity() const { return mCapacity - Size(); }

  Size_t ReduceAvailableCapacityTo(Size_t capacity) {
    // This method is like PrepForDeal, but used in a more specialized situation.
    // See TwoOpponentsGetSuit. We're distributing cards from one suit into this and another hand,
    // but will use DealUnknownsToHands to do the work. We need to temporarily reduce the capacity
    // to just the number of cards from the suit that we want to distribute to the hand.
    // After we're done, we need to restore the capacity to its previous value using RestoreCapacityTo().
    assert(capacity <= AvailableCapacity());
    Size_t prevCapacity = mCapacity;
    mCapacity = Size() + capacity;
    return prevCapacity;
  }

  void RestoreCapacityTo(Size_t capacity) {
    assert(mCapacity == Size());
    assert(capacity >= mCapacity);
    mCapacity = capacity;
  }

  // const Card Peek(unsigned i) const;

  bool HasCard(Card card) const  { return HasCard(CardBitMask(card)); }
  bool HasCard(Suit suit, Rank rank) const { return HasCard(CardBitMask(suit, rank)); }
  bool HasCard(uint64_t mask) const { return (mCardBits&mask)==mask; }

  uint64_t HasAnyCardInMask(uint64_t mask) const { return mCardBits&mask; }

  void InsertCard(Card card) {
    uint64_t mask = CardBitMask(card);
    assert((mCardBits & mask) == 0);
    mCardBits |= mask;
  }

  void RemoveCard(Card card);
    // card must exist in hand, assertion will fail if not

  inline Card FirstCard() const {
    assert(mCardBits != 0);
    return LeastSetBitIndex(mCardBits);
  }

  inline Card LastCard() const {
    assert(mCardBits != 0);
    return GreatestSetBitIndex(mCardBits);
  }

  Card NthCard(unsigned n) const;

  Card aCardAtRandom(const RandomGenerator& rng) const;

  bool operator==(const CardArray& other) const;

  CardArray CardsWithSuit(Suit suit) const;

  CardArray CardsNotWithSuit(Suit suit) const;

  static inline uint64_t SuitMask(Suit suit) { return ((1ul<<13) - 1) << (suit*13); }

  unsigned CountCardsWithMask(uint64_t mask) const { return CountBits(mCardBits & mask); }

  unsigned CountCardsWithSuit(Suit suit) const { return CountCardsWithMask(SuitMask(suit)); }

  CardArray NonPointCards() const;

  void PartitionRemaining(Suit suit, CardArray& remaininOfSuit, CardArray& otherRemaining) const;

  void Print() const;

  std::string AsString() const;

  CardArray(uint64_t bits, CardArrayInit ignored)
  : mCapacity(0)
  , mCardBits(bits)
  {
    assert(mCardBits < (1L<<52));
    mCapacity = CountBits(mCardBits);
  }

private:

  inline uint64_t CardBitMask(Suit suit, Rank rank) const {
    return 1L << (suit*13 + rank);
  }

  inline uint64_t CardBitMask(Card card) const { return 1L << card; }

protected:
  Size_t mCapacity;
    // The current capacity of the array, must be <= MaxCapacity.
    // The capacity of a hand is reduced by one after each actual play.
    // When dealing cards to hands, or assigning an arrangement of unknown cards
    // to a set of hands, we set mSize to and mCapacity to the number of cards
    // the hand should hold. We can then deal cards to the hand until mSize==mCapacity

  uint64_t mCardBits;
    // A bit mask for the cards in this CardArray.
};

typedef CardArray CardHand;
typedef CardArray CardDeck;

class CardHands
{
public:
  CardHands() {}
  CardHands(const CardHands& other) {
    for (int i=0; i<4; ++i)
      mHands[i] = other.mHands[i];
  }

  void operator=(const CardHands& other) {
    for (int i=0; i<4; ++i)
      mHands[i] = other.mHands[i];
  }

  CardHand& operator[](int i) { return mHands[i]; }
  const CardHand& operator[](int i) const { return mHands[i]; }

  unsigned TotalCapacity() const {
    unsigned total = 0;
    for (int i=0; i<4; ++i)
      total += mHands[i].AvailableCapacity();
    return total;
  }

  unsigned PlayersWithAvailableCapacity(unsigned expectedCapacity) const {
    unsigned numPlayers = 0;
    unsigned capacity = 0;
    for (int i=0; i<4; i++) {
      unsigned cap = mHands[i].AvailableCapacity();
      capacity += cap;
      if (cap > 0)
        ++numPlayers;
    }
    assert(capacity == expectedCapacity);
    assert(numPlayers >= 1);
    return numPlayers;
  }

private:
  CardHand mHands[4];
};
