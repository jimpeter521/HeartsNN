#include "lib/CardArray.h"

void CardArray::Init(const Card* cards, Size_t size)
{
  for (unsigned i=0; i<size; ++i) {
    InsertCard(cards[i]);
  }
}

void CardArray::RemoveCard(Card card)
{
  const uint64_t mask = CardBitMask(card);
  assert(HasCard(mask));
  mCardBits &= ~mask;
  --mCapacity;
}

Card CardArray::NthCard(unsigned n) const
{
  assert(n < Size());
  assert(mCardBits != 0);
  if (n == 0) {
    return FirstCard();
  }
  iterator it(*this);
  while (n>0) {
    it.next();
    --n;
  }
  return it.next();
}


Card CardArray::aCardAtRandom(RandomGenerator& rng) const
{
  unsigned choice = unsigned(rng.range64(Size()));
  return NthCard(choice);
}

bool CardArray::operator==(const CardArray& other) const
{
  // TODO: historically we didn't verify capacities were the same. Should we?
  return mCardBits == other.mCardBits;
}

void CardArray::Print() const
{
  for (Card card=0; card<52; ++card)
    if (HasCard(card))
      printf("%s ", NameOf(card));
  printf("\n");
}

std::string CardArray::AsString() const
{
  std::string result;
  for (Card card=0; card<52; ++card) {
    if (HasCard(card)) {
      const char* name = NameOf(card);
      result += name;
    }
  }
  return result;
}

void CardArray::Merge(const CardArray& other)
{
  if (other.mCardBits == 0)
    return;

  assert(AvailableCapacity() >= other.Size());

  // There must not be any overlap
  assert((mCardBits & other.mCardBits) == 0);

  mCardBits |= other.mCardBits;
}

void CardArray::Subtract(const CardArray& subset)
{
  if (subset.mCardBits == 0)
    return;

  // subset must not contain any cards not in this array
  assert((mCardBits & subset.mCardBits) == subset.mCardBits);

  mCardBits &= ~subset.mCardBits;
}

void CardArray::PartitionRemaining(Suit suit, CardArray& remaininOfSuit, CardArray& otherRemaining) const
{
  assert(suit>=0 && suit<=3);
  remaininOfSuit = CardsWithSuit(suit);
  otherRemaining = CardsNotWithSuit(suit);
}

CardArray CardArray::CardsWithSuit(Suit suit) const
{
  uint64_t mask = SuitMask(suit);
  return CardArray(mCardBits & mask, kGiven);
}

CardArray CardArray::CardsNotWithSuit(Suit suit) const
{
  uint64_t mask = SuitMask(suit);
  return CardArray(mCardBits & ~mask, kGiven);
}

CardArray CardArray::NonPointCards() const
{
  return CardArray(mCardBits&kNonPointCardsMask, kGiven);
}
