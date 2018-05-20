#include "lib/HeartsState.h"
#include "lib/Card.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

HeartsState::~HeartsState() {}

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
  mScore.fill(0);
  bzero(mPointTricks, sizeof(mPointTricks));
  VerifyHeartsState();
}

HeartsState::HeartsState(const HeartsState& other)
    : mDealIndex(other.mDealIndex)
    , mNextPlay(other.mNextPlay)
    , mLead(other.mLead)
    , mTrickSuit(other.mTrickSuit)
    , mPointsPlayed(other.mPointsPlayed)
    , mScore(other.mScore)
    , mIsVoidBits(other.mIsVoidBits)
    , mUnplayedCards(other.mUnplayedCards)
    , mTrackTrickWinsAtPlay(other.mTrackTrickWinsAtPlay)
    , mTrackTrickWinsForPlayer(other.mTrackTrickWinsForPlayer)
    , mTrackTrickWinsCounter(other.mTrackTrickWinsCounter)
{
  memcpy(mPlays, other.mPlays, sizeof(mPlays));
  memcpy(mPointTricks, other.mPointTricks, sizeof(mPointTricks));
  VerifyHeartsState();
}

void HeartsState::VerifyHeartsState() const
{
#ifndef NDEBUG
  assert(mNextPlay < 52);
  assert(mLead < 4);
  assert(mPointsPlayed <= 26);
  assert(mUnplayedCards.Size() == 52 - mNextPlay);

  const Card trickSuit = TrickSuit(); // Run for its the assertions
  if ((mNextPlay % 4) != 0)
  {
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

void HeartsState::UpdatePointsPlayed(Card card) { mPointsPlayed += PointsFor(card); }

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
  for (unsigned i = 1; i < 4; i++)
  {
    if (SuitOf(mPlays[i]) == mTrickSuit)
    {
      Rank r = RankOf(mPlays[i]);
      if (high < r)
      {
        high = r;
        winner = i;
      }
    }
  }

  // Conver the winner number here to be the actual player number.
  winner = (winner + mLead) % 4;

  if (mTrackTrickWinsCounter != 0 && mNextPlay == mTrackTrickWinsAtPlay && mTrackTrickWinsForPlayer == winner)
  {
    ++(*mTrackTrickWinsCounter);
  }

  return winner;
}

unsigned HeartsState::ScoreTrick()
{
  unsigned points = 0;
  for (unsigned i = 0; i < 4; ++i)
  {
    points += PointsFor(mPlays[i]);
    UpdatePointsPlayed(mPlays[i]);
  }
  return points;
}

unsigned HeartsState::GetScoreFor(unsigned player) const { return mScore[player]; }

void HeartsState::AddToScoreFor(unsigned player, unsigned score)
{
  if (score)
  {
    mScore[player] += score;
    mPointTricks[player] += 1;
  }
}

GameOutcome HeartsState::CheckForShootTheMoon()
{
  GameOutcome outcome;
  outcome.Set(mPointTricks, mScore);
  return outcome;
}

void HeartsState::RemoveUnplayedCard(Card card) { mUnplayedCards.RemoveCard(card); }

void HeartsState::setIsVoid(int player, Suit suit) { mIsVoidBits.setIsVoid(player, suit); }

bool HeartsState::isVoid(int player, Suit suit) const { return mIsVoidBits.isVoid(player, suit); }

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
    choices = PointsPlayed() == 0 ? hand.NonPointCards() : hand;
  }
  else
  {
    // We're not leading. If we have any cards from the trickSuit then only cards of the trickSuit are legal.
    const Suit trickSuit = TrickSuit();
    choices = hand.CardsWithSuit(trickSuit);
  }

  // Both of the above two paths could have resulted in no legal choices.
  if (choices.Size() == 0)
  {
    if (PlayNumber() < 4)
    {
      // In the first trick, we are not allowed to play a point card
      choices = hand.NonPointCards();
    }

    // In the rare chance that a player only has point cards in first trick,
    // we must check again to see if they have no legal choices
    if (choices.Size() == 0)
    {
      // In any other trick, all cards become playable
      choices = hand;
    }
  }

  if (PointsPlayed() == 26)
  {
    // When no more points are remaining to be played, all legal cards are equivalent, so just return the first
    // card.
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
  for (int i = 0; i < 4; ++i)
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

  assert(mTrickSuit == kUnknown || PlayInTrick() != 0);
  assert(mTrickSuit != kUnknown || PlayInTrick() == 0);
  assert((mTrickSuit == kUnknown && PlayInTrick() == 0) || mTrickSuit == SuitOf(mPlays[0]));

  return mTrickSuit;
}

void HeartsState::SetTrickSuit(Suit suit)
{
  assert(PlayInTrick() == 0);
  assert(mTrickSuit == kUnknown);
  assert(suit >= kClubs && suit <= kHearts);
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
  if (trickWins == 0)
  {
    mTrackTrickWinsCounter = 0;
    mTrackTrickWinsForPlayer = -1;
    mTrackTrickWinsAtPlay = -1;
  }
  else
  {
    mTrackTrickWinsCounter = trickWins;
    mTrackTrickWinsForPlayer = CurrentPlayer();

    // We call PlayInTrick() when PlayInTrick() returns 3, which happens when the bottom two bits of mNextPlay are
    // set.
    mTrackTrickWinsAtPlay = (mNextPlay | 3);
  }
}

bool HeartsState::IsCardOnTable(Card card) const
{
  for (int i = 0; i < PlayInTrick(); ++i)
  {
    if (card == mPlays[i])
      return true;
  }
  return false;
}

Card HeartsState::HighCardOnTable() const
{
  if (PlayInTrick() == 0)
  {
    assert(false);
    return -1;
  }

  Rank highRank = RankOf(mPlays[0]);
  for (int i = 1; i < PlayInTrick(); ++i)
  {
    Card card = mPlays[i];
    if (SuitOf(card) == mTrickSuit && highRank < RankOf(card))
    {
      highRank = RankOf(card);
    }
  }

  return CardFor(highRank, mTrickSuit);
}

bool HeartsState::MightCardTakeTrick(Card card) const
{
  if (PlayInTrick() == 0)
  {
    // A card leading a trick typically can take the trick.
    // If there are no unplayed cards in the suit, it's even guaranteed to take the trick.
    // But if the card is less than all unplayed cards, then it can't take the trick.
    const CardDeck unplayedInSuit = UnplayedCardsNotInHand(CurrentPlayersHand()).CardsWithSuit(SuitOf(card));
    return unplayedInSuit.Size() == 0 || card > unplayedInSuit.FirstCard();
  }
  else if (SuitOf(card) != mTrickSuit)
  {
    return false;
  }
  else
  {
    Card highCard = HighCardOnTable();
    return RankOf(card) > RankOf(highCard);
  }
}

bool HeartsState::WillCardTakeTrick(Card card) const
{
  // Card can't take trick if it isn't following suit
  if (PlayInTrick() != 0 && SuitOf(card) != mTrickSuit)
  {
    return false;
  }

  const CardDeck unplayedInSuit = UnplayedCardsNotInHand(CurrentPlayersHand()).CardsWithSuit(SuitOf(card));
  const bool cardIsHigherThanAnyUnplayed = unplayedInSuit.Size() == 0 || card > unplayedInSuit.LastCard();

  // If we are leading, we can only force taking the trick if we play a card higher than any unplayed card in suit
  if (PlayInTrick() == 0)
  {
    return cardIsHigherThanAnyUnplayed;
  }

  const Card highCard = HighCardOnTable();

  // If we are last in trick, we just have to beat the high card on table.
  if (PlayInTrick() == 3)
  {
    return RankOf(card) > RankOf(highCard);
  }

  // Otherwise, we have to beat high card on table and be higher than any unplayed
  return cardIsHigherThanAnyUnplayed && RankOf(card) > RankOf(highCard);
}

unsigned HeartsState::PointsOnTable() const
{
  unsigned points = 0;
  for (int i = 0; i < PlayInTrick(); ++i)
    points += PointsFor(mPlays[i]);
  return points;
}
