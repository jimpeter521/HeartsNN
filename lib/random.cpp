#include "lib/random.h"

#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <strings.h>

RandomGenerator::RandomGenerator()
{
  const int kNumBytes = sizeof(mS);
  int fd = open("/dev/urandom", O_RDONLY);
  assert(fd != -1);
  int actual = read(fd, mS, kNumBytes);
  close(fd);
  if (actual != kNumBytes) {
    fprintf(stderr, "Failed to read enough bytes to initialize RandomGenerator\n");
    exit(1);
  }

  mP = 0;
}

uint64_t RandomGenerator::random64()
{
  // see https://en.wikipedia.org/wiki/Xorshift
  // This is the xorshift1024star generator, which has a period of 2^1024 − 1
  const uint64_t s0 = mS[mP];
  uint64_t s1 = mS[mP = (mP + 1) & 15];
  s1 ^= s1 << 31; // a
  mS[mP] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30); // b,c
  return mS[mP] * 1181783497276652981ul;
}

uint128_t RandomGenerator::random128()
{
  uint128_t result = random64();
  return (result<<64) + random64();
}

uint64_t RandomGenerator::range64(uint64_t range)
{
  const uint64_t MAX64 = ~uint64_t(0);
  const uint64_t buckets = MAX64 / range;
  const uint64_t limit = buckets * range;
  uint64_t r = random64();
  while (r >= limit)
    r = random64();
  return r / buckets;
}

uint128_t RandomGenerator::range128(uint128_t range)
{
  const uint128_t buckets = MAX128 / range;
  const uint128_t limit = buckets * range;
  uint128_t r = random128();
  while (r >= limit)
    r = random128();
  return r / buckets;
}

RandomGenerator RandomGenerator::gRandomGenerator;

const uint128_t zero = 0;
const uint128_t RandomGenerator::MAX128 = ~zero;
