#pragma once

#include <x86intrin.h>

// Returns the bit index of the least set bit. Returns 64 when x is zero.
inline int LeastSetBitIndex(uint64_t x) {
  return  __tzcnt_u64(x);
}

// Returns the bit index of the greatest set bit. Returns -1 when x is zero.
inline int GreatestSetBitIndex(uint64_t x) {
  return  63 - __lzcnt64(x);
}

inline uint64_t IsolateLeastBit(uint64_t x) {
  return 1UL << LeastSetBitIndex(x);
}

inline uint64_t IsolateGreatestBit(uint64_t x) {
  return 1UL << GreatestSetBitIndex(x);
}

inline int CountBits(uint64_t bits) {
  return _popcnt64(bits);
}
