#include "lib/OneOpponentGetsSuit.h"

unsigned OpponentToGetCards(unsigned player, const VoidBits& voidBits, Suit suit)
{
  const unsigned kNone = 99;
  unsigned opponent = kNone;
  for (unsigned p=0; p<4; ++p)
  {
    if (p != player)
      if (!voidBits.isVoid(p, suit))
      {
        assert(opponent == kNone);
        opponent = p;
      }
  }
  assert(opponent != kNone);
  return opponent;
}

OneOpponentGetsSuit::OneOpponentGetsSuit(unsigned player, const VoidBits& voidBits, Suit suit
                        , const CardDeck& remainingOfSuit, const CardDeck& otherRemaining, const CardHands& hands)
: mPlayer(player)
, mVoidBits(voidBits)
, mSuit(suit)
, mRemainingOfSuit(remainingOfSuit)
, mOtherRemaining(otherRemaining)
, mOpponent(OpponentToGetCards(player, voidBits, suit))
, mNextStage(0)
, mHands(hands)
{
}

OneOpponentGetsSuit::~OneOpponentGetsSuit()
{
  delete mNextStage;
}

uint128_t OneOpponentGetsSuit::Possibilities() const
{
  return mNextStage->Possibilities();
}

void OneOpponentGetsSuit::ActualizePossibility(uint128_t possibility_index, CardHands& hands) const
{
  mVoidBits.VerifyVoids(hands);
  hands[mOpponent].Merge(mRemainingOfSuit);
  mVoidBits.VerifyVoids(hands);
  mNextStage->ActualizePossibility(possibility_index, hands);
}

void OneOpponentGetsSuit::AddStage(const CardDeck& other_remaining, PriorityList& prioList)
{
  CardHands hands(mHands);
  hands[mOpponent].Merge(mRemainingOfSuit);
  mNextStage = BuildAnalyzer(mPlayer, mVoidBits, prioList, other_remaining, hands);
}

void OneOpponentGetsSuit::RenderDot(std::ostream& stream) const
{
  stream << "id_" << uint64_t(this) << " [label=\"One suit(" << NameOfSuit(mSuit) << ") count(" << int(mRemainingOfSuit.Size())  <<  ")\"];" << std::endl;
  stream << "id_" << uint64_t(this) << " -> id_" << uint64_t(mNextStage) << std::endl;
  mNextStage->RenderDot(stream);
}

void OneOpponentGetsSuit::ExpectedDistribution(Distribution& distribution, CardHands& hands)
{
  assert(hands.TotalCapacity() == mRemainingOfSuit.Size() + mOtherRemaining.Size());

  // printf("DEBUG: OneOpponentGetsSuit incoming:\n");
  // distribution.Print();

  Distribution thisSuitDist;
  const uint128_t possibles = Possibilities();
  thisSuitDist.DistributeRemainingToPlayer(mRemainingOfSuit, mOpponent, possibles);
  // printf("DEBUG: thisSuitDist incoming:\n");
  // thisSuitDist.Print();

  distribution += thisSuitDist;

  hands[mOpponent].Merge(mRemainingOfSuit);

  Distribution nextDist;
  mNextStage->ExpectedDistribution(nextDist, hands);
  // printf("DEBUG: nextDist:\n");
  // nextDist.Print();

  distribution += nextDist;
  // printf("DEBUG: OneOpponentGetsSuit outgoing:\n");
  // distribution.Print();
}
