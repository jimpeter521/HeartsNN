#pragma once

#include "lib/Strategy.h"

class HumanPlayer : public Strategy
{
public:
    virtual ~HumanPlayer();
    HumanPlayer(StrategyPtr opponent = StrategyPtr(0));

    virtual Card choosePlay(const KnowableState& state, const RandomGenerator& rng) const;

    virtual Card predictOutcomes(
        const KnowableState& state, const RandomGenerator& rng, float playExpectedValue[13]) const;

private:
    StrategyPtr mOpponent;
};
