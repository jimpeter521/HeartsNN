#include "lib/combinatorics.h"
#include <assert.h>

uint128_t possibleDistinguishableDeals() {
  return combinations128(52, 13) * combinations128(39, 13) * combinations128(26, 13);
}

uint128_t combinations128(unsigned n, unsigned k)
{
  assert(k <= n);

  const unsigned U = std::max(k, n-k);
  const unsigned L = std::min(k, n-k);

  uint128_t numer = 1;
  for (unsigned N=n; N>U; --N)
    numer = numer * N;

  uint128_t denom = 1;
  for (unsigned N=L; N>1; --N)
    denom = denom * N;

  return numer/denom;
}
