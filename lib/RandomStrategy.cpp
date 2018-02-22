// lib/RandomStrategy.cpp

#include "lib/Card.h"
#include "lib/KnowableState.h"
#include "lib/random.h"
#include "lib/RandomStrategy.h"

RandomStrategy::~RandomStrategy() {}

RandomStrategy::RandomStrategy() {}

Card RandomStrategy::choosePlay(const KnowableState& knowableState, const RandomGenerator& rng) const
{
  CardHand choices = knowableState.LegalPlays();
  return choices.aCardAtRandom(rng);
}
