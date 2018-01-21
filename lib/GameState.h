#pragma once

#include "lib/Deal.h"
#include "lib/HeartsState.h"
#include "lib/Strategy.h"

class GameState : public HeartsState
{
public:
  GameState();

  GameState(const Deal& deal);

  GameState(const GameState& other);

  GameState(const GameState& other, const CardHands& hands);

  GameState(const CardHands& hands, const KnowableState& knowableState);

  void PlayGame(const Strategy** players, float finalScores[4], bool& shotTheMoon, Annotator* annotator = 0);
    // Return the final outcome with mean zero scores.
    // If the playerWithQueen is not NULL, return in it the number of the player who got the queen in this game.

  void PrintResults();

  void PlayCard(Card nextCardPlayed);
    // Advances the game to the next state

  void PlayOutGameMonteCarlo(float finalScores[4], bool& shotTheMoon, const Strategy* opponent);
    // Plays out the rest of this game using random legal moves at each step.
    // Return the final outcome with mean zero scores.

  void PrintState() const;

  unsigned CardCountFor(unsigned player) const { return mHands[player].Size(); }

  virtual const CardHand& CurrentPlayersHand() const { return mHands[CurrentPlayer()]; }

  Card NextPlay(const Strategy* currentPlayersStrategy, Annotator* annotator = 0);

  void PrintHand(int player) const { mHands[player].Print(); }

private:
  void PrintPlay(int player, Card card) const;

  void VerifyGameState() const;

private:
  CardHands mHands;
};
