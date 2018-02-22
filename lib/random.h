#pragma once

#include "lib/math.h"
#include "dlib/threads.h"

class RandomGenerator
{
public:
  RandomGenerator();

  uint64_t random64() const;

  uint64_t range64(uint64_t range) const;

  uint128_t random128() const;

  uint128_t range128(uint128_t range) const;

public:
  static const RandomGenerator& ThreadSpecific() { return gRandomGenerator.data(); }

  static uint64_t Random64() { return gRandomGenerator.data().random64(); }

  static uint64_t Range64(uint64_t range) { return gRandomGenerator.data().range64(range); }

  static uint128_t Random128() { return gRandomGenerator.data().random128(); }

  static uint128_t Range128(uint128_t range) { return gRandomGenerator.data().range128(range); }

public:
  static const uint128_t MAX128;

private:
  // The state must be seeded so that it is not everywhere zero.
  mutable uint64_t mS[16];
  mutable int mP;

private:
  static dlib::thread_specific_data<RandomGenerator> gRandomGenerator;
};
