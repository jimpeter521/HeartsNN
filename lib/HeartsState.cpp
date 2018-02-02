#include "lib/HeartsState.h"
#include "lib/Card.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

HeartsState::~HeartsState()
{}

HeartsState::HeartsState(uint128_t dealIndex)
: mDealIndex(dealIndex)
, mNextPlay(0)
, mLead(0)
, mTrickSuit(kUnknown)
, mPointsPlayed(0)
, mIsVoidBits()
, mUnplayedCards(kFull, kCardsPerDeck)
, mTrackTrickWinsAtPlay(-1)
, mTrackTrickWinsForPlayer(-1)
, mTrackTrickWinsCounter(0)
{
  bzero(mPlays, sizeof(mPlays));
  bzero(mScore, sizeof(mScore));
  bzero(mPointTricks, sizeof(mPointTricks));
  VerifyHeartsState();
}

HeartsState::HeartsState(const HeartsState& other)
: mDealIndex(other.mDealIndex)
, mNextPlay(other.mNextPlay)
, mLead(other.mLead)
, mTrickSuit(other.mTrickSuit)
, mPointsPlayed(other.mPointsPlayed)
, mIsVoidBits(other.mIsVoidBits)
, mUnplayedCards(other.mUnplayedCards)
, mTrackTrickWinsAtPlay(other.mTrackTrickWinsAtPlay)
, mTrackTrickWinsForPlayer(other.mTrackTrickWinsForPlayer)
, mTrackTrickWinsCounter(other.mTrackTrickWinsCounter)
{
  memcpy(mPlays, other.mPlays, sizeof(mPlays));
  memcpy(mScore, other.mScore, sizeof(mScore));
  memcpy(mPointTricks, other.mPointTricks, sizeof(mPointTricks));
  VerifyHeartsState();
}

void HeartsState::VerifyHeartsState() const
{
#ifndef NDEBUG
  assert(mNextPlay < 52);
  assert(mLead < 4);
  assert(mPointsPlayed <= 26);
  assert(mUnplayedCards.Size() == 52-mNextPlay);

  const Card trickSuit = TrickSuit(); // Run for its the assertions
  if ((mNextPlay%4) != 0) {
    assert(SuitOf(mPlays[0]) == trickSuit);
  }

  // TODO: checks on these members
  // Card mPlays[4];
  // unsigned mScore[4];
  // VoidBits mIsVoidBits;
#endif
}

unsigned HeartsState::CurrentPlayer() const
{
  const unsigned playInTrick = PlayInTrick();
  const unsigned currentPlayer = (PlayerLeadingTrick() + playInTrick) % 4;
  return currentPlayer;
}

void HeartsState::UpdatePointsPlayed(Card card)
{
  mPointsPlayed += PointsFor(card);
}

Card HeartsState::GetTrickPlay(unsigned i) const
{
  assert(i < 4);
  return mPlays[i];
}

void HeartsState::SetTrickPlay(unsigned i, Card card)
{
  assert(i < 4);
  mPlays[i] = card;
}

unsigned HeartsState::TrickWinner() const
{
  unsigned winner = 0;
  assert((mNextPlay % 4) == 3);
  assert(SuitOf(mPlays[0]) == mTrickSuit);
  Rank high = RankOf(mPlays[0]);
  for (unsigned i=1; i<4; i++)
  {
    if (SuitOf(mPlays[i]) == mTrickSuit)
    {
      Rank r = RankOf(mPlays[i]);
      if (high < r) {
        high = r;
        winner = i;
      }
    }
  }

  // Conver the winner number here to be the actual player number.
  winner = (winner + mLead) %4;

  if (mTrackTrickWinsCounter!=0 && mNextPlay==mTrackTrickWinsAtPlay && mTrackTrickWinsForPlayer==winner) {
    ++(*mTrackTrickWinsCounter);
  }

  return winner;
}

unsigned HeartsState::ScoreTrick()
{
  unsigned points = 0;
  for (unsigned i=0; i<4; ++i) {
    points += PointsFor(mPlays[i]);
    UpdatePointsPlayed(mPlays[i]);
  }
  return points;
}

unsigned HeartsState::GetScoreFor(unsigned player) const
{
  return mScore[player];
}

void HeartsState::AddToScoreFor(unsigned player, unsigned score)
{
  if (score) {
    mScore[player] += score;
    mPointTricks[player] += 1;
  }
}

void HeartsState::CheckForShootTheMoon(float scores[4], bool &shotTheMoon, int pointTricks[4], bool& stoppedTheMoon)
{
  const int currentPlayer = CurrentPlayer();
  const unsigned kExpectedTotal = 26u;

  memcpy(pointTricks, mPointTricks, sizeof(mPointTricks));

  int playerWithZeroPointTricks = 0;
  int playerWithOnePointTricks = 0;
  int playerWithMoreThanOnePointTricks = 0;
  int pointsInOneTricks = 0;

  unsigned total = 0;
  for (int i=0; i<4; i++)
  {
    total += mScore[i];
    if (mPointTricks[i] == 0) {
      ++playerWithZeroPointTricks;
    } else if (mPointTricks[i] == 1) {
      ++playerWithOnePointTricks;
      pointsInOneTricks += mScore[i];
    } else {
      ++playerWithMoreThanOnePointTricks;
    }
  }
  assert(total == kExpectedTotal);

  // We say that one player shot the moon if they took just one trick with one or more hearts
  // and another player took all the rest of the points.
  // If the one trick taken included the Queen of Spades, we don't count that as stopping the other player.
  // We're only interested in the two cases that involve the current player (shooting or stopping).
  // If one of the other players stops another of the other players from shooting the moon, we don't
  // count it as either.
  // The rationale is that we only track stopping the moon so that we can train the NN to take into account
  // the strategy of stopping the moon, and the risk of being stopped when their hand isn't strong enough.
  // We'll do that by using a modified scoring system, where we reward the current player for stopping, and
  // penalize the current player for being stopped. We'll keep a zero mean making the penalty offset the reward.
  // But if this offset doesn't affect the current player, we can ignore it.

  shotTheMoon = playerWithZeroPointTricks==3;
  stoppedTheMoon = playerWithZeroPointTricks==2 && mPointTricks[currentPlayer] != 0
                && playerWithOnePointTricks==1 && playerWithMoreThanOnePointTricks==1 && pointsInOneTricks<=4;

  assert(!stoppedTheMoon || !shotTheMoon);

  if (shotTheMoon)
  {
    assert(playerWithOnePointTricks == 0);
    assert(playerWithMoreThanOnePointTricks == 1);
    for (int i=0; i<4; i++) {
      mScore[i] = kExpectedTotal - mScore[i];
      scores[i] = mScore[i] - 19.5;
    }
  }
  else
  {
    for (int i=0; i<4; i++) {
      scores[i] = mScore[i] - 6.5;
    }
  }
  if (stoppedTheMoon) {
    int otherPlayer=0;
    for (; otherPlayer<4; ++otherPlayer) {
      if (otherPlayer!=currentPlayer && mPointTricks[otherPlayer]!=0)
        break;
    }
    const float kReward = -5;
    if (mPointTricks[currentPlayer] == 1) {
      // Yay, we stopped another player
      scores[currentPlayer] += kReward;
      scores[otherPlayer] -= kReward;
    } else {
      // Boo, we were stopped by other player
      scores[currentPlayer] -= kReward;
      scores[otherPlayer] += kReward;
    }
  }



}

void HeartsState::RemoveUnplayedCard(Card card)
{
  mUnplayedCards.RemoveCard(card);
}

void HeartsState::setIsVoid(int player, Suit suit)
{
  mIsVoidBits.setIsVoid(player, suit);
}

bool HeartsState::isVoid(int player, Suit suit) const
{
  return mIsVoidBits.isVoid(player, suit);
}

CardDeck HeartsState::UnplayedCardsNotInHand(const CardHand& myHand) const
{
  CardDeck result(mUnplayedCards);
  result.Subtract(myHand);
  return result;
  // return mUnplayedCards.Select([myHand](Card card) -> bool { return !myHand.HasCard(card); });
}

PriorityList HeartsState::MakePriorityList(unsigned player, const CardDeck& remaining) const
{
  return mIsVoidBits.MakePriorityList(player, remaining);
}

CardHand HeartsState::LegalPlays() const
{
  // VerifyHeartsState();

  const int play = PlayNumber();
  const CardHand& hand = CurrentPlayersHand();

  CardHand choices;
  if (play == 0)
  {
    Card twoOfClubs = CardFor(kTwo, kClubs);
    assert(hand.FirstCard() == twoOfClubs);
    choices.InsertCard(twoOfClubs);
    return choices;
  }

  if (PlayInTrick() == 0)
  {
    // This hand leads for this trick
    // If no points have been played, we can't lead with a card with points
    choices = PointsPlayed()==0 ? hand.NonPointCards() : hand;
  }
  else
  {
    // We're not leading. If we have any cards from the trickSuit then only cards of the trickSuit are legal.
    const Suit trickSuit = TrickSuit();
    assert(trickSuit>=kClubs && trickSuit<=kHearts);
    choices = hand.CardsWithSuit(trickSuit);
  }

  // Both of the two paths could have resulted in in legal choices. When that happens, all cards in hand are legal.
  if (choices.Size() == 0)
    choices = hand;

  if (PointsPlayed()==26)
  {
    // When no more points are remaining to be played, all legal cards are equivalent, so just return the first card.
    Card card = choices.FirstCard();
    CardHand hand;
    hand.InsertCard(card);
    assert(hand.Size() == 1);
    assert(hand.FirstCard() == card);
    choices = hand;
  }

  assert(choices.Size() > 0);
  return choices;
}

bool HeartsState::PointsSplit() const
{
  int playersWithPoints = 0;
  for (int i=0; i<4; ++i)
    if (mScore[i] != 0)
      ++playersWithPoints;
  return playersWithPoints > 1;
}

Suit HeartsState::TrickSuit() const
{
  // if (mNextPlay == 0) {
  //   assert(mTrickSuit == kClubs);
  //   return mTrickSuit;
  // }

  assert(mTrickSuit==kUnknown || PlayInTrick()!=0);
  assert(mTrickSuit!=kUnknown || PlayInTrick()==0);
  assert((mTrickSuit==kUnknown && PlayInTrick()==0) || mTrickSuit==SuitOf(mPlays[0]));

  return mTrickSuit;
}

void HeartsState::SetTrickSuit(Suit suit)
{
  assert(PlayInTrick() == 0);
  assert(mTrickSuit == kUnknown);
  assert(suit>=kClubs && suit<=kHearts);
  mTrickSuit = suit;
}

void HeartsState::AdvancePlayNumber()
{
  ++mNextPlay;
  if (PlayInTrick() == 0)
    mTrickSuit = kUnknown;
}

void HeartsState::TrackTrickWinner(unsigned* trickWins)
{
  if (trickWins == 0) {
    mTrackTrickWinsCounter = 0;
    mTrackTrickWinsForPlayer = -1;
    mTrackTrickWinsAtPlay = -1;
  } else {
    mTrackTrickWinsCounter = trickWins;
    mTrackTrickWinsForPlayer = CurrentPlayer();

    // We call PlayInTrick() when PlayInTrick() returns 3, which happens when the bottom two bits of mNextPlay are set.
    mTrackTrickWinsAtPlay = (mNextPlay | 3);
  }
}

Card HeartsState::HighCardOnTable() const
{
  if (PlayInTrick() == 0) {
    assert(false);
    return -1;
  }

  Rank highRank = RankOf(mPlays[0]);
  for (int i=1; i<PlayInTrick(); ++i) {
    Card card = mPlays[i];
    if (SuitOf(card)==mTrickSuit && highRank<RankOf(card)) {
      highRank = RankOf(card);
    }
  }

  return CardFor(highRank, mTrickSuit);
}


bool HeartsState::MightCardTakeTrick(Card card) const
{
  if (PlayInTrick() == 0) {
    // A card leading a trick typically can take the trick.
    // If there are no unplayed cards in the suit, it's even guaranteed to take the trick.
    // But if the card is less than all unplayed cards, then it can't take the trick.
    const CardDeck unplayedInSuit = UnplayedCardsNotInHand(CurrentPlayersHand()).CardsWithSuit(SuitOf(card));
    return unplayedInSuit.Size()==0 || card > unplayedInSuit.FirstCard();
  }
  else if (SuitOf(card) != mTrickSuit) {
    return false;
  }
  else {
    Card highCard = HighCardOnTable();
    return RankOf(card) > RankOf(highCard);
  }
}
