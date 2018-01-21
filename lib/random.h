#pragma once

#include "lib/math.h"

class RandomGenerator
{
public:
  RandomGenerator();

  uint64_t random64();

  uint64_t range64(uint64_t range);

  uint128_t random128();

  uint128_t range128(uint128_t range);

public:
  static RandomGenerator gRandomGenerator;

  static const uint128_t MAX128;

private:
  // The state must be seeded so that it is not everywhere zero.
  uint64_t mS[16];
  int mP;
};
