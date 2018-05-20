#pragma once

#include "lib/Card.h"
#include "lib/CardArray.h"
#include "lib/GameOutcome.h"
#include "lib/VoidBits.h"

#include <array>

// HeartsState is an abstract base class, for implementation classes KnowableState and GameState.
// HeartsState should only contain "knowable" information, i.e. information that any of the 4 players
// can rightfully know about the state of the game.
// KnowableState adds the information known to the current player.
// GameState is the full game state, known only to an omniscient and impartial referee.
//
// HeartsState contains one bit of information that should not be knowable to any of the players,
// -- the dealIndex, which uniquely identifies the deal used to seed the game.
// But the only way we use dealIndex is to replay a game later while debugging or analyzing performance.
// We keep the dealIndex here just for convenience.

class HeartsState
{
public:
  virtual ~HeartsState();

  HeartsState(uint128_t dealIndex);
  HeartsState(const HeartsState& other);

  uint128_t dealIndex() const { return mDealIndex; }

  // Play Number
  unsigned PlayNumber() const { return mNextPlay; }
  void AdvancePlayNumber();
  bool Done() const { return mNextPlay == 52; }

  // Trick Suit
  Suit TrickSuit() const;
  void SetTrickSuit(Suit suit);

  Card HighCardOnTable() const;
  bool MightCardTakeTrick(Card card) const;
  // false if the card is not in trick suit or is less than the high card in trick so far.
  // A true means the card is not ruled out from taking trick, but does not guarantee it will.

  bool WillCardTakeTrick(Card card) const;
  // True if this legal play card is guaranteed to take the current trick.
  // False if card is not in trick suit, or is less than unplayed cards in the suit.

  bool IsCardOnTable(Card card) const;
  // True if card is currently face up on table in current trick

  unsigned PointsOnTable() const;
  // Return the number of points for cards currently face up on the table

  // Trick relative
  unsigned PlayerLeadingTrick() const { return mLead; }
  unsigned PlayInTrick() const { return mNextPlay % 4; }
  unsigned CurrentPlayer() const; // (mLead + playInTrick) % 4;
  void SetLead(int player) { mLead = player; }

  // Total points played
  unsigned PointsPlayed() const { return mPointsPlayed; }
  bool PointsSplit() const;
  void UpdatePointsPlayed(Card card);

  Card GetTrickPlay(unsigned i) const;
  void SetTrickPlay(unsigned i, Card card);

  unsigned ScoreTrick();

  // Player Score tracking
  unsigned GetScoreFor(unsigned player) const;
  void AddToScoreFor(unsigned player, unsigned score);
  GameOutcome CheckForShootTheMoon();

  // Known voids
  // bool areAnyPlayersKnownVoid() const { return mIsVoidBits.areAnyPlayersKnownVoid(); }
  VoidBits IsVoidBits() const { return mIsVoidBits; }
  bool isVoid(int player, Suit suit) const;
  void setIsVoid(int player, Suit suit);
  PriorityList MakePriorityList(unsigned player, const CardDeck& remaining) const;

  // Unplayed Cards
  const CardDeck& UnplayedCards() const { return mUnplayedCards; }
  void RemoveUnplayedCard(Card card);
  CardDeck UnplayedCardsNotInHand(const CardHand& myHand) const;

  // Legal Plays
  CardHand LegalPlays() const;

  virtual const CardHand& CurrentPlayersHand() const = 0;

  void VerifyHeartsState() const;

  void TrackTrickWinner(unsigned* trickWins);

  const std::array<unsigned, 4>& PointsSoFar() const { return mScore; }

protected:
  // Returns the player number of the player who wins the trick
  unsigned TrickWinner() const;

  // Sets the new lead based upon the trick winner
  unsigned NewLead(int winner)
  {
    mLead = winner;
    return mLead;
  }

private:
  const uint128_t mDealIndex;
  unsigned mNextPlay;
  unsigned mLead;
  Suit mTrickSuit;
  unsigned mPointsPlayed;
  Card mPlays[4];

  std::array<unsigned, 4> mScore;
  // This is the number of points the player has won so far 0..26

  unsigned mPointTricks[4];
  // This is the number of tricks that the player won in which points were taken.
  // When a player shoots the moon, three of the four slots will be 0.
  // When a player nearly shot the moon but was stopped, two will be zero, one will be 1 (and other will be >1).

  VoidBits mIsVoidBits;
  CardDeck mUnplayedCards;

  int mTrackTrickWinsAtPlay;
  int mTrackTrickWinsForPlayer;
  unsigned* mTrackTrickWinsCounter;
};
