#pragma once

#include "lib/Deal.h"
#include "lib/GameState.h"
#include "lib/HeartsState.h"
#include "lib/Strategy.h"
#include "lib/VisibleState.hpp"

typedef std::function<void(int, int, Card)> PlayCardHook;
typedef std::function<void(int, std::array<unsigned, 4>)> TrickResultHook;

class GameState : public HeartsState
{
public:
  GameState();

  GameState(const Deal& deal);

  GameState(const GameState& other);

  GameState(const GameState& other, const CardHands& hands);

  GameState(const CardHands& hands, const KnowableState& knowableState);

  void SetPlayCardHook(PlayCardHook hook) { mPlayCardHook = hook; }
  void SetTrickResultHook(TrickResultHook hook) { mTrickResultHook = hook; }

  GameOutcome PlayGame(StrategyPtr players[4], const RandomGenerator& rng);
  // Return the final outcome with mean zero scores.
  // If the playerWithQueen is not NULL, return in it the number of the player who got the queen in this game.

  void PrintResults();

  void PlayCard(Card nextCardPlayed);
  // Advances the game to the next state

  GameOutcome PlayOutGameMonteCarlo(const StrategyPtr& opponent, const RandomGenerator& rng);
  // Plays out the rest of this game using random legal moves at each step.
  // Return in finalScores the final outcome with mean zero scores.
  // Return in pointTricks the count of points-with-tricks won by each player

  void PrintState() const;

  unsigned CardCountFor(unsigned player) const { return mHands[player].Size(); }

  virtual const CardHand& CurrentPlayersHand() const { return mHands[CurrentPlayer()]; }

  Card NextPlay(const StrategyPtr& currentPlayersStrategy, const RandomGenerator& rng);

  void AdvanceOnePlay(StrategyPtr players[4], const RandomGenerator& rng);

  void PrintHand(int player) const { mHands[player].Print(); }

  const CardHand& HandForPlayer(unsigned player) const { return mHands[player]; }

  visible::VisibleState asVisibleState(unsigned player) const;

private:
  void PrintPlay(int player, Card card) const;

  void VerifyGameState() const;

private:
  CardHands mHands;
  PlayCardHook mPlayCardHook{nullptr};
  TrickResultHook mTrickResultHook{nullptr};
};
