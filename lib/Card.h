// lib/Card.h

#pragma once

#include <stdint.h>

typedef uint8_t Card;
typedef uint8_t Rank;
typedef uint8_t Suit;

enum Suits
{
  kClubs = 0u,
  kDiamonds,
  kSpades,
  kHearts,

  kUnknown = 99u
};

enum Ranks {
  kTwo = 0u,
  kThree,
  kFour,
  kFive,
  kSix,
  kSeven,
  kEight,
  kNine,
  kTen,
  kJack,
  kQueen,
  kKing,
  kAce
};

const uint8_t kSuitsPerDeck = 4u;
const uint8_t kCardsPerSuit = 13u;
const uint8_t kCardsPerHand = 13u;
const uint8_t kCardsPerDeck = kSuitsPerDeck * kCardsPerSuit;
const unsigned kNumPlayers = 4u;

inline Suit SuitOf(Card card) { return card / kCardsPerSuit; }
inline Rank RankOf(Card card) { return card % kCardsPerSuit; }
inline Card CardFor(Rank rank, Suit suit) { return suit*kCardsPerSuit + rank; }

const char* NameOfSuit(Suit suit);
const char* NameOf(Card card);

unsigned PointsFor(Card card);

const uint64_t kAllCardsMask = (1ul<<kCardsPerDeck) - 1;
const uint64_t kAllHeartsMask = ((1ul<<kCardsPerSuit)-1) << (kHearts*kCardsPerSuit);
const uint64_t kPointCardsMask = kAllHeartsMask | (1ul << CardFor(kQueen, kSpades));
const uint64_t kNonPointCardsMask = (~kPointCardsMask) & kAllCardsMask;
