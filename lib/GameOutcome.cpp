// GameOutcome.cpp

#include "lib/GameOutcome.h"

#include <assert.h>
#include <string.h>

const unsigned kExpectedTotal = 26u;

GameOutcome::GameOutcome()
: mShooter(-1)
{

}

static int firstWith(unsigned a[4], unsigned val) {
  for (int i=0; i<4; i++) {
    if (a[i] == val) {
      return i;
    }
  }
  assert(false);
  return -1;
}

static int firstWithMany(unsigned a[4]) {
  for (int i=0; i<4; i++) {
    if (a[i] > 1) {
      return i;
    }
  }
  assert(false);
  return -1;
}

void GameOutcome::Set(unsigned pointTricks[4], unsigned score[4])
{
  const unsigned kNumBytes = sizeof(mPointTricks);
  assert(kNumBytes == sizeof(mScores));
  memcpy(mPointTricks, pointTricks, kNumBytes);
  memcpy(mScores, score, kNumBytes);

  // We're going to count the number of players who took zero tricks with points, one trick with points,
  // and more than one trick with points.
  int withZeroTricks = 0;
  int withOneTrick = 0;
  int withMultipleTricks = 0;

  // We accumulate the number of points taken by players who took exactly one trick.
  // We only use this value in the case where it appears that a player may have stopped another player.
  // We do this only because we don't consider it a successful stop if the player took the queen of spades.
  int pointsInOneTricks = 0;

  unsigned total = 0;
  for (int i=0; i<4; i++)
  {
    total += mScores[i];
    if (mPointTricks[i] == 0) {
      ++withZeroTricks;
    } else if (mPointTricks[i] == 1) {
      ++withOneTrick;
      pointsInOneTricks += mScores[i];
    } else {
      assert(mPointTricks[i] > 1);
      ++withMultipleTricks;
    }
  }
  assert(total == kExpectedTotal);
  assert(4 == withZeroTricks+withOneTrick+withMultipleTricks);

  // We can tell that one player shot the moon when the other three players took no points
  mShotTheMoon = withZeroTricks==3;

  if (mShotTheMoon) {
    mShooter = firstWith(mScores, kExpectedTotal);
  }
}

void GameOutcome::updateMoonStats(unsigned currentPlayer, int iChoice, int moonCounts[13][kNumMoonCountKeys]) const
{
  if (mShotTheMoon) {
    unsigned myScore = mScores[currentPlayer];
    assert(myScore <= 26u);
    if (myScore == kExpectedTotal) {
      ++moonCounts[iChoice][kCurrentShotTheMoon];
    } else {
      assert(mShooter!=-1 && mShooter!=currentPlayer);
      ++moonCounts[iChoice][kOtherShotTheMoon];
    }
  }
}

float GameOutcome::boringScore(unsigned currentPlayer) const {
  assert(mScores[currentPlayer] <= 26u);
  float score = mScores[currentPlayer];
  assert(score >= 0.0);
  assert(score <= 26.0);
  return score;
}

float GameOutcome::standardScore(unsigned currentPlayer) const
{
  assert(mScores[currentPlayer] <= 26u);
  if (!mShotTheMoon)
  {
    return boringScore(currentPlayer) - 6.5;
  }
  else
  {
    if (mScores[currentPlayer] == 26u)
      return -19.5;
    else
      return 6.5;
  }
}
