// GameOutcome.cpp

#include "lib/GameOutcome.h"

#include <assert.h>
#include <string.h>

const unsigned kExpectedTotal = 26u;

GameOutcome::GameOutcome(int stopTheMoonPenalty)
: mStopTheMoonPenalty(stopTheMoonPenalty)
, mShooter(-1)
, mStopper(-1)
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

  // Detecting that one player stopped another is harder, as there is multiple conditions we have to check.
  mStoppedTheMoon = withZeroTricks==2
                && withOneTrick==1 && withMultipleTricks==1 && pointsInOneTricks<=4;

  // It's not possible that in the same game that both someone shot the moon and someone stopped a player from shooting.
  assert(!mStoppedTheMoon || !mShotTheMoon);

  if (mShotTheMoon) {
    mShooter = firstWith(mScores, kExpectedTotal);
  } else if (mStoppedTheMoon) {
    mStopper = firstWith(mPointTricks, 1);
    mShooter = firstWithMany(mPointTricks);
  }
}

void GameOutcome::updateMoonStats(unsigned currentPlayer, int iChoice, int moonCounts[13][kNumMoonCountKeys])
{
  if (mShotTheMoon) {
    unsigned myScore = mScores[currentPlayer];
    if (myScore == kExpectedTotal) {
      ++moonCounts[iChoice][kCurrentShotTheMoon];
    } else {
      assert(mShooter!=-1 && mShooter!=currentPlayer);
      ++moonCounts[iChoice][kOtherShotTheMoon];
    }
  } else if (mStoppedTheMoon) {
    unsigned myPointTricks = mPointTricks[currentPlayer];
    if (myPointTricks == 1) {
      ++moonCounts[iChoice][kCurrentStoppedTheMoon];
    } else if (myPointTricks > 1) {
      ++moonCounts[iChoice][kOtherStoppedTheMoon];
    }
  }
}

float GameOutcome::boringScore(unsigned currentPlayer) const {
  return mScores[currentPlayer] - 6.5;
}

float GameOutcome::modifiedScore(unsigned currentPlayer) const
{
  float score = boringScore(currentPlayer);
  if (mShotTheMoon) {
    unsigned myScore = mScores[currentPlayer];
    if (myScore == kExpectedTotal) {
      score -= 39.0;
    } else {
      score += 13.0;
    }
  }
  else if (mStoppedTheMoon && mPointTricks[currentPlayer]!=0) {
    if (mPointTricks[currentPlayer] == 1) {
      score -= mStopTheMoonPenalty;
    } else {
      score += mStopTheMoonPenalty;
    }
  }
  return score;
}
