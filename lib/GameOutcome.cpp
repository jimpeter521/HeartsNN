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

// Record a GameOutcome. The `score` array here comes directly from the GameState, and is simply
// the total number of points for each player, which always sums to 26.
void GameOutcome::Set(unsigned pointTricks[4], unsigned score[4])
{
  const unsigned kNumBytes = sizeof(mPointTricks);
  assert(kNumBytes == sizeof(mScores));
  memcpy(mPointTricks, pointTricks, kNumBytes);
  memcpy(mScores, score, kNumBytes);

  // We're going to count the number of players who took zero tricks with points, one trick with points,
  // and more than one trick with points.
  int withZeroTricks = 0;
  int withMultipleTricks = 0;

  unsigned total = 0;
  for (int i=0; i<4; i++)
  {
    total += mScores[i];
    if (mPointTricks[i] == 0) {
      ++withZeroTricks;
    } else {
      assert(mPointTricks[i] > 0);
      ++withMultipleTricks;
    }
  }
  assert(total == kExpectedTotal);
  assert(4 == withZeroTricks+withMultipleTricks);

  // We can tell that one player shot the moon when the other three players took no points
  mShotTheMoon = withZeroTricks==3;

  if (mShotTheMoon) {
    mShooter = firstWith(mScores, kExpectedTotal);
  }
}

void GameOutcome::updateMoonStats(unsigned currentPlayer, int iChoice, unsigned moonCounts[13][kNumMoonCountKeys]) const
{
  if (mShotTheMoon) {
    unsigned myScore = mScores[currentPlayer];
    assert(myScore <= kExpectedTotal);
    if (myScore == kExpectedTotal) {
      ++moonCounts[iChoice][kCurrentShotTheMoon];
    } else {
      assert(mShooter!=-1 && mShooter!=currentPlayer);
      ++moonCounts[iChoice][kOtherShotTheMoon];
    }
  }
}

unsigned GameOutcome::PointsTaken(unsigned currentPlayer) const {
  unsigned points = mScores[currentPlayer];
  assert(points <= kExpectedTotal);
  return points;
}

float GameOutcome::ZeroMeanStandardScore(unsigned currentPlayer) const
{
  unsigned pointsTaken = PointsTaken(currentPlayer);
  if (!mShotTheMoon)
  {
    assert(pointsTaken != kExpectedTotal);
    return float(pointsTaken) - 6.5;
  }
  else
  {
    if (pointsTaken == kExpectedTotal)
      return -19.5;
    else {
      assert(pointsTaken == 0);
      return 6.5;
    }
  }
}
