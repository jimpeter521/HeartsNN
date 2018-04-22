// lib/RandomStrategy.cpp

#include "lib/RandomStrategy.h"
#include "lib/Card.h"
#include "lib/KnowableState.h"
#include "lib/random.h"

RandomStrategy::~RandomStrategy() {}

RandomStrategy::RandomStrategy() {}

Card RandomStrategy::choosePlay(const KnowableState& knowableState, const RandomGenerator& rng) const
{
    CardHand choices = knowableState.LegalPlays();
    return choices.aCardAtRandom(rng);
}

Card RandomStrategy::predictOutcomes(
    const KnowableState& state, const RandomGenerator& rng, float playExpectedValue[13]) const
{
    for (int i = 0; i < 13; i++)
        playExpectedValue[i] = 0.0;
    return choosePlay(state, rng);
}
