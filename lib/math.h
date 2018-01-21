#pragma once

#include <string>

typedef __uint128_t uint128_t;

std::string asDecimalString(uint128_t N);
  // This is inefficient, but it's meant for unit tests and other performance non-critical code

std::string asHexString(uint128_t N, int zeroFillTo=0);
// If the output would have less than zeroFileTo hex digits, pad on the left with zeros.
// This is inefficient, but it's meant for unit tests and other performance non-critical code


uint128_t parseHex128(const char* hexString);
