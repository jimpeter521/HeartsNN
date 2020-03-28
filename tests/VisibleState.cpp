#include "gtest/gtest.h"

#include "lib/VisibleState.hpp"
#include "lib/Deal.h"

TEST(VisibleState, empty) {
    auto state = visible::VisibleState::Builder()
        .build();

    const auto hand = state.hand();
    EXPECT_EQ(hand.Size(), 0);
}

TEST(VisibleState, oneCardInHand) {
    const auto state = visible::VisibleState::Builder()
        .add(TheQueen())
        .build();

    const auto hand = state.hand();
    EXPECT_EQ(hand.Size(), 1);
}

TEST(VisibleState, deal) {
    Deal deal;
    auto dealt = deal.dealFor(0);

    const auto state = visible::VisibleState::Builder()
        .add(dealt)
        .build();

    const auto hand = state.hand();
    EXPECT_EQ(hand.Size(), 13);
}
