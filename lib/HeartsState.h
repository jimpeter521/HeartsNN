#pragma once

#include "lib/Card.h"
#include "lib/VoidBits.h"
#include "lib/CardArray.h"

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

class HeartsState {
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

  // Trick relative
  unsigned PlayerLeadingTrick() const { return mLead; }
  unsigned PlayInTrick() const { return mNextPlay % 4; }
  unsigned CurrentPlayer() const;  // (mLead + playInTrick) % 4;
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
  void CheckForShootTheMoon(float scores[4], bool &shotTheMoon);

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

protected:
  // Returns the player number of the player who wins the trick
  unsigned TrickWinner() const;

  // Sets the new lead based upon the trick winner
  unsigned NewLead(int winner) { mLead = winner; return mLead; }

private:
  const uint128_t mDealIndex;
  unsigned mNextPlay;
  unsigned mLead;
  Suit mTrickSuit;
  unsigned mPointsPlayed;
  Card mPlays[4];
  unsigned mScore[4];
  VoidBits mIsVoidBits;
  CardDeck mUnplayedCards;

  int       mTrackTrickWinsAtPlay;
  int       mTrackTrickWinsForPlayer;
  unsigned* mTrackTrickWinsCounter;
};
