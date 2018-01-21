#include "gtest/gtest.h"
#include "lib/random.h"
#include "lib/combinatorics.h"

#include <algorithm>

TEST(random, max128) {
  const uint128_t max = RandomGenerator::MAX128;

  // Adding one to max causes rollover to zero
  uint128_t maxPlus1 = max + 1;
  EXPECT_EQ(0, maxPlus1);

  // Subtracting one from zero causes rollover to max
  const uint128_t zero = 0;
  uint128_t zeroMinus1 = zero - 1;
  EXPECT_EQ(max, zeroMinus1);

  // asHexString reports max is 32 fs.
  const std::string hex = asHexString(max);
  const std::string expected(32, 'f');
  EXPECT_EQ(expected, hex);
}

TEST(random, range128top8Bits) {
  RandomGenerator gen;

  const unsigned kBins = 8;
  unsigned bins[kBins] = { 0, 0, 0, 0, 0, 0, 0, 0 };

  const uint128_t D = possibleDistinguishableDeals();
  const uint128_t B = D / kBins;

  const int kIterations = 100000;
  for (int i=0; i<kIterations; i++) {
    uint128_t r = gen.range128(D);
    unsigned x = unsigned(r / B);
    EXPECT_LT(x, kBins);
    ++bins[x];
  }

  const double kScale = double(kBins) / double(kIterations);
  std::sort(bins, bins+8);

  const double lo = double(bins[0]) * kScale;
  const double hi = double(bins[kBins-1]) * kScale;
  EXPECT_GT(lo, 0.97);
  EXPECT_LT(hi, 1.03);
  printf("range128top8Bits %f %f\n", lo, hi);
}

TEST(random, range128bot8Bits) {
  RandomGenerator gen;

  const unsigned kBins = 8;
  unsigned bins[kBins] = { 0, 0, 0, 0, 0, 0, 0, 0 };

  const uint128_t D = possibleDistinguishableDeals();

  const int kIterations = 100000;
  for (int i=0; i<kIterations; i++) {
    uint128_t r = gen.range128(D);
    unsigned x = r % kBins;
    ++bins[x];
  }

  const double kScale = double(kBins) / double(kIterations);
  std::sort(bins, bins+8);

  const double lo = double(bins[0]) * kScale;
  const double hi = double(bins[kBins-1]) * kScale;
  EXPECT_GT(lo, 0.97);
  EXPECT_LT(hi, 1.03);
  printf("range128bot8Bits %f %f\n", lo, hi);
}

