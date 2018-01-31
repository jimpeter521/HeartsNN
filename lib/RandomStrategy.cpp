// lib/RandomStrategy.cpp

#include "lib/Card.h"
#include "lib/KnowableState.h"
#include "lib/random.h"
#include "lib/RandomStrategy.h"

static RandomGenerator rng;

const RandomStrategy* const RandomStrategy::gSingleton = new RandomStrategy;

RandomStrategy::~RandomStrategy() {}

RandomStrategy::RandomStrategy() {}

Card RandomStrategy::choosePlay(const KnowableState& knowableState) const
{
  CardHand choices = knowableState.LegalPlays();
  return choices.aCardAtRandom(rng);
}
