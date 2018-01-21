#include "gtest/gtest.h"

#include "lib/combinatorics.h"
#include "lib/Deal.h"

TEST(Deal, defaultConstructor) {
  Deal deal;
}

TEST(Deal, constructWithRandomIndex) {
  uint128_t index = Deal::RandomDealIndex();
  Deal deal(index);
}

TEST(Deal, deckForIndex0) {
  Deal deal(0);
  for (int i=0; i<52; i++)
    EXPECT_EQ(i, deal.PeekAt(i));
}

TEST(Deal, deckForIndex1) {
  Deal deal(1);
  for (int i=0; i<38; i++)
    EXPECT_EQ(i, deal.PeekAt(i));
  for (int i=40; i<52; i++)
    EXPECT_EQ(i, deal.PeekAt(i));
  EXPECT_EQ(39, deal.PeekAt(38));
  EXPECT_EQ(38, deal.PeekAt(39));
}

TEST(Deal, deckForIndex2) {
  Deal deal(2);
  for (int i=0; i<38; i++)
    EXPECT_EQ(i, deal.PeekAt(i));
  for (int i=41; i<52; i++)
    EXPECT_EQ(i, deal.PeekAt(i));
  EXPECT_EQ(40, deal.PeekAt(38));
  EXPECT_EQ(38, deal.PeekAt(39));
  EXPECT_EQ(39, deal.PeekAt(40));
}

TEST(Deal, deckWithLastIndex) {
  const uint128_t index = possibleDistinguishableDeals() - 1;

  Deal deal(index);
  for (int suit=0; suit<4; suit++) {
    for (int rank=0; rank<13; rank++) {
      int i = suit*13 + rank;
      int expectedSuit = 3 - suit;
      int expectedCard = expectedSuit * 13 + rank;
      EXPECT_EQ(expectedCard, deal.PeekAt(i));
    }
  }
}
