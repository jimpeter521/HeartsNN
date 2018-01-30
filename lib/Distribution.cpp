#include "lib/Distribution.h"
#include "lib/Card.h"
#include "lib/combinatorics.h"

#include <assert.h>
#include <string.h>
#include <strings.h>

Distribution::Distribution()
{
  bzero(mCounts, sizeof(mCounts));
}

Distribution::Distribution(const Distribution& other)
{
  *this = other;
}

void Distribution::operator=(const Distribution& other)
{
  assert(this != &other);
  memcpy(mCounts, other.mCounts, sizeof(mCounts));
}

void Distribution::operator+=(const Distribution& other)
{
  for (int c=0; c<52; ++c)
    for (int p=0; p<4; ++p)
      mCounts[c][p] += other.mCounts[c][p];
}

void Distribution::operator*=(uint128_t multiplier)
{
  for (int c=0; c<52; ++c)
    for (int p=0; p<4; ++p)
      mCounts[c][p] *= multiplier;
}

bool Distribution::operator==(const Distribution& other) const
{
  for (int c=0; c<52; ++c)
    for (int p=0; p<4; ++p)
      if (mCounts[c][p] != other.mCounts[c][p])
        return false;
  return true;
}


void Distribution::DistributeRemaining(const CardDeck& remaining, uint128_t possibles, const CardHands& hands)
{
  const uint128_t total = hands.TotalCapacity();
  assert(total == remaining.Size());
  unsigned cardsLeft = total;
  uint128_t ways = 1;
  unsigned capacities[4];
  for (int p=0; p<4; ++p) {
    capacities[p] = hands[p].AvailableCapacity();
    ways *= combinations128(cardsLeft, capacities[p]);
    cardsLeft -= capacities[p];
  }
  assert(ways == possibles);

  CardArray::iterator it(remaining);
  while (!it.done()) {
    Card card = it.next();
    for (int p=0; p<4; ++p) {
      if (capacities[p])
        mCounts[card][p] += possibles * capacities[p] / total;
    }
  }
}

void Distribution::CountOccurrences(const CardHands& hands)
{
  for (int player=0; player<4; player++) {
    const CardHand& hand = hands[player];
    CardArray::iterator it(hand);
    while (!it.done()) {
      Card card = it.next();
      mCounts[card][player] += 1;
    }
  }
}

void Distribution::Print() const
{
  for (Card c=0; c<52; c++) {
    uint128_t total = 0;
    for (int p=0; p<4; ++p) {
      total += mCounts[c][p];
    }
    if (total > 0) {
      printf("%3s %5s %5s %5s %5s\n", NameOf(c)
        , asDecimalString(mCounts[c][0]).c_str()
        , asDecimalString(mCounts[c][1]).c_str()
        , asDecimalString(mCounts[c][2]).c_str()
        , asDecimalString(mCounts[c][3]).c_str());
    }
  }
}

void Distribution::AsProbabilities(float prob[52][4]) const
{
  uint128_t possibilities = 0;
  float scale = 0.0;
  for (Card c=0; c<52; ++c) {
    uint128_t sum = 0;
    for (int p=0; p<4; ++p) {
      sum += mCounts[c][p];
    }
    if (sum>0 && possibilities==0) {
      possibilities = sum;
      scale = 1.0 / float(possibilities);
    }
    if (sum == 0) {
      for (int p=0; p<4; ++p) {
        prob[c][p] = 0.0;
      }
    } else {
      assert(sum == possibilities);
      for (int p=0; p<4; ++p) {
        prob[c][p] = scale * mCounts[c][p];
      }
    }
  }
}

static bool NonZero(float prob[4]) {
  float sum = 0.0;
  for (int p=0; p<4; ++p) {
    sum += prob[p];
  }
  if (sum == 0.0)
    return false;

  assert (sum > 0.999 &&  sum < 1.001);
  return true;
}

void Distribution::PrintProbabilities(float prob[52][4], FILE* out)
{
  for (Card c=0; c<52; ++c)
  {
    if (NonZero(prob[c])) {
      fprintf(out, "%3s ", NameOf(c));
      for (int p=0; p<4; ++p)
        fprintf(out, " %5.3f", prob[c][p]);
      fprintf(out, "\n");
    }
  }
}

void Distribution::Validate(const CardDeck& allUnplayed, uint128_t possibilities) const
{
#ifndef NDEBUG
  for (Card c=0; c<52; c++) {
    uint128_t total = 0;
    for (int p=0; p<4; ++p) {
      total += mCounts[c][p];
    }
    if (total > 0) {
      assert(total == possibilities);
      assert(allUnplayed.HasCard(c));
    }
    else {
      assert(!allUnplayed.HasCard(c));
    }
  }
#endif
}

void Distribution::DistributeRemainingToPlayer(const CardArray& remaining, unsigned player, uint128_t possibles)
{
  CardArray::iterator it(remaining);
  while (!it.done()) {
    Card card = it.next();
    mCounts[card][player] += possibles;
  }
}
