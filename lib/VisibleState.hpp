// lib/VisibleState.h

#pragma once

#include "lib/Card.h"
#include "lib/CardArray.h"

namespace visible {

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

    using Plays = std::array<Card, 4>;

    // see below for public accessors

    class Builder
    {
    public:
        Builder() = default;

        Builder& add(Card card) { mHand.Insert(card); return *this; }

        Builder& add(const CardArray& cards)
        {
            CardHand::iterator it(cards);
            while (!it.done()) { add(it.next()); }
            return *this;
        }

        Builder& south(Card card) { mPlays[SOUTH] = card; return *this; }
        Builder& west(Card card) { mPlays[WEST] = card; return *this; }
        Builder& north(Card card) { mPlays[NORTH] = card; return *this; }
        Builder& east(Card card) { mPlays[EAST] = card; return *this; }

        Builder& played(int player, Card card) { mPlays[player] = card; return *this; }

        VisibleState build() const { return VisibleState(*this); }

    private:
        friend class VisibleState;
        CardHand mHand;
        Plays mPlays{kNoCard, kNoCard, kNoCard, kNoCard};
    };

private:
    VisibleState() = delete;
    VisibleState(const Builder& builder) : mHand(builder.mHand), mPlays(builder.mPlays) {}

private:
    // The cards in the human player's hand, always in the South seat
    const CardHand mHand;

    // The cards played so far in this trick
    const Plays mPlays;

public:

    const CardHand hand() const { return mHand; }
    const Plays plays() const { return mPlays; }
    const Card south() const { return mPlays[SOUTH]; }
    const Card west() const { return mPlays[WEST]; }
    const Card north() const { return mPlays[NORTH]; }
    const Card east() const { return mPlays[EAST]; }
};

} // namespace visible
