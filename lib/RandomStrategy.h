#pragma once

#include "lib/CardArray.h"
#include "lib/Strategy.h"

class RandomStrategy : public Strategy
{
public:
    virtual ~RandomStrategy();

    RandomStrategy();

    virtual Card choosePlay(const KnowableState& state, const RandomGenerator& rng) const;

    virtual Card predictOutcomes(
        const KnowableState& state, const RandomGenerator& rng, float playExpectedValue[13]) const;
};
