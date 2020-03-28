// lib/VisibleState.h

#pragma once

#include "lib/Card.h"

namespace visible {

struct Card
{
    Card(Nib r, Nib s) : rank{r}, suit{s} {}

    Card() = delete;
    ~Card() = default;

    Card(const Card&) = default;
    Card(Card&&) = default;
    Card& operator=(const Card&) = delete;
    Card& operator=(Card&&) = delete;

    static constexpr Nib kRanks{13};

    static Card make(Ord ordinal)
    {
        Nib rank = ordinal % kRanks;
        Nib suit = ordinal / kRanks;
        return Card{rank, suit};
    }

    Ord ord() const { return suit*kRanks + rank; }

    const Nib rank;
    const Nib suit;
};

using OptionalCard = std::optional<Card>;
using OptionalOrd = std::optional<Ord>;

class VisibleState
{
public:
    enum Seats : char
    {
        SOUTH = 0,
        WEST = 1,
        NORTH = 2,
        EAST = 3,
    };

    class Builder
    {
    public:
        Builder() = default;

        Builder& add(Card card) { mHand.insert(card.ord()); return *this; }

        Builder& south(Card card) { mTrickPlays[SOUTH] = std::make_optional<Ord>(card.ord()); return *this; }
        Builder& west(Card card) { mTrickPlays[WEST] = std::make_optional<Ord>(card.ord()); return *this; }
        Builder& north(Card card) { mTrickPlays[NORTH] = std::make_optional<Ord>(card.ord()); return *this; }
        Builder& east(Card card) { mTrickPlays[EAST] = std::make_optional<Ord>(card.ord()); return *this; }

        VisibleState build() const { return VisibleState(*this); }

    private:
        friend class VisibleState;
        std::set<Ord> mHand;
        std::array<OptionalOrd, 4> mTrickPlays;
    };

private:
    VisibleState() = delete;
    VisibleState(const Builder& builder) : mHand(builder.mHand.begin(), builder.mHand.end()), mTrickPlays(builder.mTrickPlays) {}

private:
    // The cards in the human player's hand, always in the South seat
    const std::vector<Ord> mHand;

    // The cards played so far in this trick
    const std::array<OptionalOrd, 4> mTrickPlays;
};

} // namespace visible
