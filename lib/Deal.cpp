#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <utility>
#include <string>
#include <cstdint>

using namespace std;

#include "lib/Deal.h"
#include "lib/combinatorics.h"
#include "lib/random.h"

static uint128_t kPossibleDistinguishableDeals = possibleDistinguishableDeals();

int Deal::startPlayer() const
{
  // The 2 clubs is card number 0
  // The hands are always sorted, so we only need to look at first card of each hand
  for (int i=0; i<4; i++)
  {
    if (PeekAt(i, 0) == 0)
      return i;
  }
  assert(false);
  return -1;
}

Deal::Deal()
: mDealIndex(RandomDealIndex())
{
  DealHands(mDealIndex);
}

Deal::Deal(uint128_t index)
: mDealIndex(index)
{
  DealHands(mDealIndex);
}

static RandomGenerator gRand;

uint128_t Deal::RandomDealIndex()
{
  return gRand.range128(kPossibleDistinguishableDeals);
}

void Deal::DealHands(uint128_t I)
{
  DealUnknownsToHands(CardDeck(kFull, kCardsPerDeck), mHands, I);
}

void ValidateDealUnknowns(const CardDeck& unknowns, const CardHands& hands)
{
  unsigned T = 0;
  for (int i=0; i<4; i++)
  {
    unsigned avail = hands[i].AvailableCapacity();
    assert(avail <= 13);
    T += avail;
  }
  assert(T == unknowns.Size());
}

uint128_t PossibleDealUnknownsToHands(const CardDeck& unknowns, const CardHands& hands)
{
  ValidateDealUnknowns(unknowns, hands);

  uint128_t result = 1;

  unsigned D = unknowns.Size();
  for (int i=0; i<4; i++) {
    unsigned H = hands[i].AvailableCapacity();
//     printf("D:%u H:%u, i:%d\n", D, H, i);
    assert(H <= D);
    result *= combinations128(D, H);
    D -= H;
  }
  assert(D == 0);
  return result;
}

void DealUnknownsToHands(const CardDeck& unknowns, CardHands& hands)
{
  const uint128_t kPossibleDeals = PossibleDealUnknownsToHands(unknowns, hands);
  uint128_t index = gRand.range128(kPossibleDeals);
  DealUnknownsToHands(unknowns, hands, index);
}

void DealUnknownsToHands(const CardDeck& unknowns, CardHands& hands, uint128_t index)
{
  ValidateDealUnknowns(unknowns, hands);

  // This code is adapted from http://www.rpbridge.net/7z68.htm
  const uint128_t kPossibleDeals = PossibleDealUnknownsToHands(unknowns, hands);
  assert(index < kPossibleDeals);

  CardArray::iterator it(unknowns);

  uint128_t K = kPossibleDeals;
  for (unsigned C = unknowns.Size(); C>0; --C) {
    uint128_t X = 0;
    for (int i=0; i<4; i++) {
      index -= X;
      X = (K * hands[i].AvailableCapacity()) / C;
      if (index < X) {
        hands[i].Insert(it.next());
        break;
      }
    }
    K = X;
  }
}

void Deal::printDeal() const
{
  for (int p=0; p<4; p++) {
    for (int i=0; i<kCardsPerSuit; i++)
      printf("%s ", NameOf(PeekAt(p, i)));
    printf("\n");
  }
  printf("\n");
}

CardHand Deal::dealFor(int player) const
{
  return mHands[player];
}
