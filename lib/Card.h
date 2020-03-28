// lib/Card.h

#pragma once

#include <array>
#include <cstdint>

using Nib = std::int8_t;   // We only need 4 bits, but we use 8. For representing Rank and Suit.
using Ord = std::uint8_t;  // We only need 6 bits, but we use 8. For values 0..51.

// We represent one card as an unsigned integer in the range 0..51
using Card = Ord;

enum Suits : Nib
{
    kClubs = 0u,
    kDiamonds,
    kSpades,
    kHearts,

    kUnknown = 99u
};

using Suit = Suits;
constexpr auto allSuits = std::array<Suit, 4>{kClubs, kDiamonds, kSpades, kHearts};

enum Ranks : Nib
{
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

using Rank = Ranks;

const unsigned kSuitsPerDeck = 4u;
const unsigned kCardsPerSuit = 13u;
const unsigned kCardsPerHand = 13u;
const unsigned kCardsPerDeck = kSuitsPerDeck * kCardsPerSuit;
const unsigned kNumPlayers = 4u;
const unsigned kMaxPointsPerHand = 26u;

inline Suit SuitOf(Card card) { return Suit(card / kCardsPerSuit); }
inline Rank RankOf(Card card) { return Rank(card % kCardsPerSuit); }
inline Card CardFor(Rank rank, Suit suit) { return suit * kCardsPerSuit + rank; }

inline Card TheQueen() { return CardFor(kQueen, kSpades); }

const char* NameOfSuit(Suit suit);
const char* NameOf(Card card);

unsigned PointsFor(Card card);

const uint64_t kAllCardsMask = (1ul << kCardsPerDeck) - 1;
const uint64_t kAllHeartsMask = ((1ul << kCardsPerSuit) - 1) << (kHearts * kCardsPerSuit);
const uint64_t kPointCardsMask = kAllHeartsMask | (1ul << CardFor(kQueen, kSpades));
const uint64_t kNonPointCardsMask = (~kPointCardsMask) & kAllCardsMask;
