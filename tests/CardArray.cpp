#include "gtest/gtest.h"

#include "lib/CardArray.h"

static const Card cards[] = { 0, 3, 6, 10, 51 };
static const int kNumCards = sizeof(cards)/sizeof(cards[0]);

TEST(CardArray, default_ctor) {
  CardHand hand;
  EXPECT_EQ(hand.Size(), 0);
}

TEST(CardArray, Init) {
  CardHand hand;
  hand.Init(cards, kNumCards);
  EXPECT_EQ(hand.Size(), kNumCards);
}

TEST(CardArray, full_ctor) {
  CardDeck deck(kFull, 52);
  EXPECT_EQ(52, deck.Size());
  EXPECT_EQ(52, deck.Capacity());
  EXPECT_EQ(0, deck.AvailableCapacity());
}

TEST(CardArray, assignment) {
  CardHand hand;
  hand.Init(cards, kNumCards);

  CardHand other;
  other = hand;
  EXPECT_EQ(other.Size(), kNumCards);
}

TEST(CardArray, Append) {
  CardHand hand;
  EXPECT_EQ(0, hand.Size());

  hand.Append(3);
  EXPECT_EQ(1, hand.Size());
  EXPECT_EQ(3, hand.FirstCard());

  hand.Append(7);
  EXPECT_EQ(2, hand.Size());
  EXPECT_EQ(7, hand.LastCard());
}

TEST(CardArray, Insert) {
  CardHand hand;

  Card cards[] = { 1, 7, 0};
  for (int i=0; i<3; i++)
    hand.Insert(cards[i]);

  EXPECT_EQ(3, hand.Size());
  for (int i=0; i<3; i++)
    EXPECT_TRUE(hand.HasCard(cards[i]));

  EXPECT_EQ(0, hand.FirstCard());
  EXPECT_EQ(7, hand.LastCard());
}

TEST(CardArray, Merge1) {
  Card cards1[] = { 0, 3, 6, 14, 21, 51 };
  CardHand hand1;
  hand1.Init(cards1, 6);
  Card cards2[] = { 1, 2, 5, 15, 22, 50 };
  CardHand hand2;
  hand2.Init(cards2, 6);

  hand1.Merge(hand2);

  Card e[] = {0, 1, 2, 3, 5, 6, 14, 15, 21, 22, 50, 51};
  CardHand expected;
  expected.Init(e, 12);
  EXPECT_EQ(expected, hand1);
}

TEST(CardArray, Merge2) {
  Card cards1[] = { 0, 3, 6, 14, 21, 51 };
  CardHand hand1;
  hand1.Init(cards1, 6);
  Card cards2[] = { 1, 2, 5, 15, 22, 50 };
  CardHand hand2;
  hand2.Init(cards2, 6);

  hand2.Merge(hand1);

  Card e[] = {0, 1, 2, 3, 5, 6, 14, 15, 21, 22, 50, 51};
  CardHand expected;
  expected.Init(e, 12);
  EXPECT_EQ(expected, hand2);
}
