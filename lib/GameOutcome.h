#pragma once


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

enum MoonCountKey {
  kCurrentShotTheMoon = 0,
  kOtherShotTheMoon = 1,
  kNumMoonCountKeys = 2
};

class GameOutcome
{
public:
  GameOutcome();

  GameOutcome(const GameOutcome& other);

  void Set(unsigned pointTricks[4], unsigned score[4]);

  void updateMoonStats(unsigned currentPlayer, int iChoice, int moonCounts[13][kNumMoonCountKeys]) const;
    // Moon counts is an aggregation across many outcomes for many legal play choices.
    // These method updates moonCounts for the this one outcome

  float boringScore(unsigned currentPlayer) const;
    // Return the score for the boring game of hearts, i.e. one without shooting the moon.
    // This score is in the range of 0.0 to 26.0.

  float standardScore(unsigned currentPlayer) const;
    // Return the standard score, taking into account shooting the moon, but with zero mean.
    // This score is in the range of -19.5 to 18.5.

  const bool shotTheMoon() const { return mShotTheMoon; }

private:
  unsigned mScores[4];
  unsigned mPointTricks[4];
  bool mShotTheMoon;
  int mShooter;
};
