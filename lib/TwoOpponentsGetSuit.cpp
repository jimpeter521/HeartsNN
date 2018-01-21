#include "lib/TwoOpponentsGetSuit.h"
#include "lib/combinatorics.h"
#include "lib/Deal.h"

TwoOpponents TwoOpponentsToGetCards(unsigned player, const VoidBits& voidBits, Suit suit)
{
  TwoOpponents opponents;
  unsigned* opp = opponents.p;
  for (unsigned p=0; p<4; ++p)
    if (p != player)
      if (!voidBits.isVoid(p, suit))
        *opp++ = p;

  assert(opp - opponents.p == 2);

  return opponents;
}


TwoOpponentsGetSuit::TwoOpponentsGetSuit(unsigned player, const VoidBits& voidBits, Suit suit
                            , const CardDeck& remainingOfSuit, const CardDeck& otherRemaining, const CardHands& hands)
: mPlayer(player)
, mVoidBits(voidBits)
, mSuit(suit)
, mRemainingOfSuit(remainingOfSuit)
, mOtherRemaining(otherRemaining)
, mOpponents(TwoOpponentsToGetCards(player, voidBits, suit))
{
  const unsigned remaining = remainingOfSuit.Size();
  for (unsigned i=0; i<=remaining; ++i) {
    PossibilityAnalyzer* ways = 0;
    if (hands[mOpponents.p[0]].AvailableCapacity() < i)
      ways = new Impossible();
    else if (hands[mOpponents.p[1]].AvailableCapacity() < remaining-i)
      ways = new Impossible();
    else
      ways = new Ways(mPlayer, mVoidBits, suit, mRemainingOfSuit, mOtherRemaining, hands, mOpponents, i);
    mWays.push_back(ways);
  }
}

TwoOpponentsGetSuit::~TwoOpponentsGetSuit()
{
  for (auto it = mWays.begin(); it!=mWays.end(); ++it) {
    PossibilityAnalyzer* way = (*it);
    delete way;
  }
}

uint128_t TwoOpponentsGetSuit::Possibilities() const
{
  uint128_t possibilities = 0;
  for (auto it = mWays.begin(); it!=mWays.end(); ++it)
    possibilities += (*it)->Possibilities();
  return possibilities;
}

void TwoOpponentsGetSuit::ActualizePossibility(uint128_t possibility_index, CardHands& hands) const
{
  mVoidBits.VerifyVoids(hands);
  for (auto it=mWays.begin(); it!=mWays.end(); ++it) {
    PossibilityAnalyzer* way = (*it);
    const uint128_t wayPossibles = way->Possibilities();
    if (possibility_index < wayPossibles) {
      way->ActualizePossibility(possibility_index, hands);
      break;
    }
    possibility_index -= wayPossibles;
  }
  mVoidBits.VerifyVoids(hands);
}

void TwoOpponentsGetSuit::AddStage(const CardDeck& other_remaining, PriorityList& list)
{
  const unsigned remaining = mRemainingOfSuit.Size();
  for (unsigned i=0; i<=remaining; ++i) {
    PriorityList prioList(list);
    mWays[i]->AddStage(other_remaining, prioList);
  }
}

void TwoOpponentsGetSuit::RenderDot(std::ostream& stream) const
{
  stream << "id_" << uint64_t(this) << " [label=\"Two get suit(" << NameOfSuit(mSuit) << ") count(" << int(mRemainingOfSuit.Size())  <<  ")\"];" << std::endl;
  const unsigned remaining = mRemainingOfSuit.Size();
  for (unsigned i=0; i<=remaining; ++i)
  {
    stream << "id_" << uint64_t(this) << " -> id_" << uint64_t(mWays[i]) << ";" << std::endl;
    mWays[i]->RenderDot(stream);
  }
}

void TwoOpponentsGetSuit::ExpectedDistribution(Distribution& distribution, CardHands& hands)
{
  assert(hands.TotalCapacity() == mRemainingOfSuit.Size() + mOtherRemaining.Size());

  // printf("DEBUG: TwoOpponentsGetSuit incoming:\n");
  // distribution.Print();

  for (auto it=mWays.begin(); it!=mWays.end(); ++it) {
    PossibilityAnalyzer* way = (*it);
    if (way->Possibilities() > 0) {
      Distribution nextDist;
      CardHands nextHands(hands);
      way->ExpectedDistribution(nextDist, nextHands);
      distribution += nextDist;
    }
  }

  // printf("DEBUG: TwoOpponentsGetSuit outgoing:\n");
  // distribution.Print();
}


Ways::Ways(unsigned player, const VoidBits& voidBits, Suit suit, const CardDeck& remainingOfSuit
        , const CardDeck& otherRemaining, const CardHands& hands, const TwoOpponents& opponents, unsigned numFirstPlayer)
: mPlayer(player)
, mVoidBits(voidBits)
, mSuit(suit)
, mRemainingOfSuit(remainingOfSuit)
, mOtherRemaining(otherRemaining)
, A(opponents.p[0])
, B(opponents.p[1])
, mNumFirstPlayer(numFirstPlayer)
, mNextStage(0)
, mHands(hands)
{
  assert(A != B);
  assert(A != mPlayer);
  assert(mPlayer != B);

  assert(mHands[A].AvailableCapacity() + mHands[B].AvailableCapacity() >= remainingOfSuit.Size());
}

Ways::~Ways()
{
  delete mNextStage;
}

uint128_t Ways::Possibilities() const
{
  const uint128_t arrangements = combinations128(mRemainingOfSuit.Size(), mNumFirstPlayer);
  const uint128_t nextPossibles = mNextStage->Possibilities();
  return arrangements * nextPossibles;
}

void Ways::ActualizePossibility(uint128_t possibilityIndex, CardHands& hands) const
{
  mVoidBits.VerifyVoids(hands);

  const uint128_t nextPossibles = mNextStage->Possibilities();
  uint128_t iArrangement = possibilityIndex / nextPossibles;

  unsigned prevCapacities[4];
  for (int i=0; i<4; i++) {
    unsigned newCapacity = 0;
    if (i==A) {
      newCapacity = mNumFirstPlayer;
    } else if (i==B) {
      newCapacity = mRemainingOfSuit.Size() - mNumFirstPlayer;
    }
    prevCapacities[i] = hands[i].ReduceAvailableCapacityTo(newCapacity);
  }

  DealUnknownsToHands(mRemainingOfSuit, hands, iArrangement);

  mVoidBits.VerifyVoids(hands);

  for (int i=0; i<4; i++) {
    hands[i].RestoreCapacityTo(prevCapacities[i]);
  }

  possibilityIndex = possibilityIndex % nextPossibles;
  mNextStage->ActualizePossibility(possibilityIndex, hands);
  mVoidBits.VerifyVoids(hands);
}

void Ways::AddStage(const CardDeck& other_remaining, PriorityList& prioList)
{
  CardHands hands(mHands);

  const unsigned remaining = mRemainingOfSuit.Size();
  CardArray::iterator it(mRemainingOfSuit);
  for (unsigned i=0; i<remaining; ++i) {
    const int index = int(i >= mNumFirstPlayer);
    assert(index==0 || index==1);
    const unsigned opponent = index==0 ? A : B;
    CardHand& hand = hands[opponent];
    if (hand.AvailableCapacity() >= 1) {
      hand.Insert(it.next());
    } else {
      assert(hand.AvailableCapacity() == 0);
      mNextStage = new Impossible();
      return;
    }
  }

  mNextStage = BuildAnalyzer(mPlayer, mVoidBits, prioList, other_remaining, hands);
}

void Ways::RenderDot(std::ostream& stream) const
{
  stream << "id_" << uint64_t(this) << " [label=\"Ways suit(" << NameOfSuit(mSuit) << ") count(" << int(mRemainingOfSuit.Size())  <<  ")\"];" << std::endl;
  stream << "id_" << uint64_t(this) << " -> id_" << uint64_t(mNextStage) << std::endl;
  mNextStage->RenderDot(stream);
}

void Ways::ExpectedDistribution(Distribution& distribution, CardHands& hands)
{
  assert(hands.TotalCapacity() == mRemainingOfSuit.Size() + mOtherRemaining.Size());

  if (mNextStage->Possibilities() == 0)
    return;

  assert(hands[A].AvailableCapacity() + hands[B].AvailableCapacity() >= mRemainingOfSuit.Size());
  assert(hands[A].AvailableCapacity() >= mNumFirstPlayer);
  assert(hands[B].AvailableCapacity() >= mRemainingOfSuit.Size()-mNumFirstPlayer);

  // printf("DEBUG: Ways incoming:\n");
  // distribution.Print();

  Distribution thisSuitDist;
  uint128_t possibles = Possibilities();
  if (mNumFirstPlayer == 0) {
    thisSuitDist.DistributeRemainingToPlayer(mRemainingOfSuit, B, possibles);
  } else if (mNumFirstPlayer == mRemainingOfSuit.Size()) {
    thisSuitDist.DistributeRemainingToPlayer(mRemainingOfSuit, A, possibles);
  } else {
    thisSuitDist.DistributeRemainingToPlayer(mRemainingOfSuit, A, (possibles * mNumFirstPlayer) / mRemainingOfSuit.Size());
    thisSuitDist.DistributeRemainingToPlayer(mRemainingOfSuit, B, (possibles * (mRemainingOfSuit.Size()-mNumFirstPlayer)) / mRemainingOfSuit.Size());
  }
  // printf("DEBUG: thisSuitDist incoming:\n");
  // thisSuitDist.Print();
  distribution += thisSuitDist;


  const unsigned remaining = mRemainingOfSuit.Size();
  CardArray::iterator it(mRemainingOfSuit);
  for (unsigned i=0; i<remaining; ++i) {
    const int index = int(i >= mNumFirstPlayer);
    assert(index==0 || index==1);
    const unsigned opponent = index==0 ? A : B;
    CardHand& hand = hands[opponent];
    assert(hand.AvailableCapacity() >= 1);
    hand.Insert(it.next());
  }

  Distribution nextDist;
  mNextStage->ExpectedDistribution(nextDist, hands);
  nextDist *= combinations128(mRemainingOfSuit.Size(), mNumFirstPlayer);
  distribution += nextDist;

  // printf("DEBUG: Ways outgoing:\n");
  // distribution.Print();
}
