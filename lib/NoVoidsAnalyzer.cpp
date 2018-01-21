#include "lib/NoVoidsAnalyzer.h"
#include "lib/Deal.h"

NoVoidsAnalyzer::NoVoidsAnalyzer(const CardDeck& remaining, const CardHands& hands, const VoidBits& voidBits)
: mRemaining(remaining)
, mHands(hands)
, mVoidBits(voidBits)
{
  mVoidBits.VerifyVoids(mHands);

  // for (int suit=0; suit<4; ++suit) {
  //   if (remaining.CountCardsWithSuit(suit) > 0) {
  //     assert(mVoidBits.CountVoidInSuit(suit) == 0);
  //   }
  // }
}

NoVoidsAnalyzer::~NoVoidsAnalyzer()
{
}

uint128_t NoVoidsAnalyzer::Possibilities() const
{
  return PossibleDealUnknownsToHands(mRemaining, mHands);
}

void NoVoidsAnalyzer::ActualizePossibility(uint128_t possibility_index, CardHands& hands) const
{
  DealUnknownsToHands(mRemaining, hands, possibility_index);
}

void NoVoidsAnalyzer::AddStage(const CardDeck& other_remaining, PriorityList& list)
{
  assert(false);
}

void NoVoidsAnalyzer::RenderDot(std::ostream& stream) const
{
  stream << "id_" << uint64_t(this) << " [label=\"NoVoids count(" << int(mRemaining.Size())  <<  ")\"];" << std::endl;
}

void NoVoidsAnalyzer::ExpectedDistribution(Distribution& distribution, CardHands& hands)
{
  assert(hands.TotalCapacity() == mRemaining.Size());

  // printf("DEBUG: NoVoidsAnalyzer incoming:\n");
  // distribution.Print();

  distribution.DistributeRemaining(mRemaining, Possibilities(), mHands);

  // printf("DEBUG: NoVoidsAnalyzer outgoing:\n");
  // distribution.Print();
}
