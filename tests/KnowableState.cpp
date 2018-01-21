#include "gtest/gtest.h"

#include "lib/KnowableState.h"
#include "lib/GameState.h"

TEST(KnowableState, nominal) {
  GameState gameState;
  KnowableState knowableState(gameState);
}

TEST(KnowableState, HypotheticalState) {
  GameState gameState;
  KnowableState knowableState(gameState);
  GameState derived = knowableState.HypotheticalState();
}
