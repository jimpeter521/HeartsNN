#pragma once

#include "CardArray.h"

typedef __uint128_t uint128_t;

class Distribution
{
public:
  Distribution();
  Distribution(const Distribution& other);
  void operator=(const Distribution& other);
  void operator+=(const Distribution& other);
  void operator*=(uint128_t multiplier);

  bool operator==(const Distribution& other) const;

  void DistributeRemaining(const CardDeck& remaining, uint128_t possibles, const CardHands& hands);
    // possibles should already be scaled correctly, i.e. total possibles divided by players with available capacity
    // Called from NoVoidsAnalyzer

  void DistributeRemainingToPlayer(const CardArray& remaining, unsigned player, uint128_t possibles);
    // All of these remaining cards go to one player. Possibles must include all possible distributions of other cards.
    // Called from OneOpponentGetsSuit

  void CountOccurrences(const CardHands& hands);
    // Given one sample hands configuration, count the occurences, i.e. increment each (card, player) seen.
    // Called from disttest

  void Print() const;

  void AsProbabilities(float prob[52][4]) const;

  static void PrintProbabilities(float prob[52][4], FILE* out=stdout);

  void Validate(const CardDeck& allUnplayed, uint128_t possibilities) const;

private:
  uint128_t mCounts[52][4];
};
