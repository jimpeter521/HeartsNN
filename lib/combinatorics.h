#pragma once

#include <string>
#include "lib/math.h"

uint128_t combinations128(unsigned n, unsigned k);
  // Returns n things taken k at a time.
  // No attempt is made to detect overflow, but 128 bits is big enough for Hearts.

uint128_t possibleDistinguishableDeals();
  // Returns 52! / 13!^4
