#include "gtest/gtest.h"

#include "lib/Card.h"
#include "lib/CardArray.h"
#include "lib/Bits.h"

TEST(Card, constants) {
  EXPECT_EQ(4u, kSuitsPerDeck);
  EXPECT_EQ(13u, kCardsPerSuit);
  EXPECT_EQ(52u, kCardsPerDeck);
}

TEST(Card, CardFor) {
  EXPECT_EQ(0u, CardFor(kTwo, kClubs));
  EXPECT_EQ(13u, CardFor(kTwo, kDiamonds));
  EXPECT_EQ(26u, CardFor(kTwo, kSpades));
  EXPECT_EQ(39u, CardFor(kTwo, kHearts));

  EXPECT_EQ(12u, CardFor(kAce, kClubs));
  EXPECT_EQ(25u, CardFor(kAce, kDiamonds));
  EXPECT_EQ(38u, CardFor(kAce, kSpades));
  EXPECT_EQ(51u, CardFor(kAce, kHearts));
}

TEST(Card, SuitOf) {
  EXPECT_EQ(kClubs, SuitOf(0u));
  EXPECT_EQ(kDiamonds, SuitOf(13u));
  EXPECT_EQ(kSpades, SuitOf(26u));
  EXPECT_EQ(kHearts, SuitOf(39u));

  EXPECT_EQ(kClubs, SuitOf(12u));
  EXPECT_EQ(kDiamonds, SuitOf(25u));
  EXPECT_EQ(kSpades, SuitOf(38u));
  EXPECT_EQ(kHearts, SuitOf(51u));
}

TEST(Card, RankOf) {
  EXPECT_EQ(kTwo, RankOf(0u));
  EXPECT_EQ(kTwo, RankOf(13u));
  EXPECT_EQ(kTwo, RankOf(26u));
  EXPECT_EQ(kTwo, RankOf(39u));

  EXPECT_EQ(kAce, RankOf(12u));
  EXPECT_EQ(kAce, RankOf(25u));
  EXPECT_EQ(kAce, RankOf(38u));
  EXPECT_EQ(kAce, RankOf(51u));
}

TEST(Card, NameOf) {
  EXPECT_STREQ( " 2♣️", NameOf(CardFor(kTwo, kClubs)));
  EXPECT_STREQ( " 2♦️", NameOf(CardFor(kTwo, kDiamonds)));
  EXPECT_STREQ( " 2♠️", NameOf(CardFor(kTwo, kSpades)));
  EXPECT_STREQ( " 2♥️", NameOf(CardFor(kTwo, kHearts)));

  EXPECT_STREQ( "10♣️", NameOf(CardFor(kTen, kClubs)));
  EXPECT_STREQ( "10♦️", NameOf(CardFor(kTen, kDiamonds)));
  EXPECT_STREQ( "10♠️", NameOf(CardFor(kTen, kSpades)));
  EXPECT_STREQ( "10♥️", NameOf(CardFor(kTen, kHearts)));

  EXPECT_STREQ( " A♣️", NameOf(CardFor(kAce, kClubs)));
  EXPECT_STREQ( " A♦️", NameOf(CardFor(kAce, kDiamonds)));
  EXPECT_STREQ( " A♠️", NameOf(CardFor(kAce, kSpades)));
  EXPECT_STREQ( " A♥️", NameOf(CardFor(kAce, kHearts)));
}

TEST(Card, PointsFor) {
  EXPECT_EQ(0, PointsFor(CardFor(kTwo, kClubs)));
  EXPECT_EQ(0, PointsFor(CardFor(kTwo, kDiamonds)));
  EXPECT_EQ(0, PointsFor(CardFor(kTwo, kSpades)));
  EXPECT_EQ(1, PointsFor(CardFor(kTwo, kHearts)));

  EXPECT_EQ(0, PointsFor(CardFor(kQueen, kClubs)));
  EXPECT_EQ(0, PointsFor(CardFor(kQueen, kDiamonds)));
  EXPECT_EQ(13, PointsFor(CardFor(kQueen, kSpades)));
  EXPECT_EQ(1, PointsFor(CardFor(kQueen, kHearts)));
}

TEST(Card, PointCardMask) {
  EXPECT_EQ(14, CountBits(kPointCardsMask));
  CardArray pointCards(kPointCardsMask, kGiven);
  EXPECT_EQ(14, pointCards.Size());

  CardArray::iterator it(pointCards);
  int pointsSum = 0;
  while (!it.done()) {
    Card card = it.next();
    EXPECT_NE(0, PointsFor(card));
    pointsSum += PointsFor(card);
  }
  EXPECT_EQ(26, pointsSum);
}

TEST(Card, NonPointCardMask) {
  EXPECT_EQ(38, CountBits(kNonPointCardsMask));
  CardArray nonPointCards(kNonPointCardsMask, kGiven);
  EXPECT_EQ(38, nonPointCards.Size());

  CardArray::iterator it(nonPointCards);
  while (!it.done()) {
    Card card = it.next();
    EXPECT_EQ(0, PointsFor(card));
  }

  const uint64_t check = kPointCardsMask | kNonPointCardsMask;
  EXPECT_EQ((1ul<<52) - 1, check);
  EXPECT_EQ(52, CountBits(check));
}
