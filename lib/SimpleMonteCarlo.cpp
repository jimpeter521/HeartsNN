#include "lib/Card.h"
#include "lib/GameState.h"
#include "lib/KnowableState.h"
#include "lib/random.h"
#include "lib/RandomStrategy.h"
#include "lib/SimpleMonteCarlo.h"
#include "lib/PossibilityAnalyzer.h"

#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <algorithm>

static RandomGenerator rng;

SimpleMonteCarlo::~SimpleMonteCarlo() {
}

SimpleMonteCarlo::SimpleMonteCarlo(const StrategyPtr& intuition, const AnnotatorPtr& annotator, uint128_t maxAlternates)
: Strategy(annotator)
, mIntuition(intuition)
, kMaxAlternates(maxAlternates)
{
}

static bool floatEqual(float a, float b) {
  return fabsf(a-b) < 0.1;
}

static void updateMoonStats(unsigned currentPlayer, int iChoice, float finalScores[4], int moonCounts[13][2]) {
  float myScore = finalScores[currentPlayer];
  if (floatEqual(myScore, -19.5)) {
    ++moonCounts[iChoice][0];
  } else {
    assert(floatEqual(myScore, 6.5));
    ++moonCounts[iChoice][1];
  }
}

// For each legal play, play out (roll out) the game many times
// Compute the expected score of a play as the average score all game rollouts.

Card SimpleMonteCarlo::choosePlay(const KnowableState& knowableState) const
{
  // knowableState.VerifyHeartsState();

  const unsigned currentPlayer = knowableState.CurrentPlayer();
  const CardHand choices = knowableState.LegalPlays();

  if (choices.Size()==1)
    return choices.FirstCard();

  assert(knowableState.PointsPlayed() < 26);

  float scores[13][4] = {{0.0}};
  int moonCounts[13][2] = {{0}}; // mc[i][0] is I shot the moon, mc[i][1] is other shot the moon
  unsigned trickWins[13] = {0};

  PossibilityAnalyzer* analyzer = knowableState.Analyze();
  const uint128_t numPossibilities = analyzer->Possibilities();
  const bool exhaustive = numPossibilities < kMaxAlternates;
  const unsigned numAlternates = exhaustive ? numPossibilities : kMaxAlternates;

  // For each possible alternate arrangement of opponent's unplayed cards
  for (unsigned alternate=0; alternate<numAlternates; ++alternate)
  {
    const uint128_t possibilityIndex = exhaustive ? alternate : rng.range128(numPossibilities);

    CardHands hands;
    knowableState.PrepareHands(hands);
    analyzer->ActualizePossibility(possibilityIndex, hands);

    knowableState.IsVoidBits().VerifyVoids(hands);

    // Construct the game state for this alternate
    const GameState alt(hands, knowableState);

    CardArray::iterator it(choices);

    // For each possible play
    for (unsigned i=0; i<choices.Size(); ++i)
    {
      Card nextCardPlayed = it.next();

      // Construct the next game state
      GameState next(alt);
      next.TrackTrickWinner(trickWins + i);
      next.PlayCard(nextCardPlayed);

      float finalScores[4];

      // Do one "roll out", i.e. play out the game to the end, using random plays
      bool shotTheMoon;
      next.PlayOutGameMonteCarlo(finalScores, shotTheMoon, mIntuition);

      next.TrackTrickWinner(0);
      if (shotTheMoon)
        updateMoonStats(currentPlayer, i, finalScores, moonCounts);

      for (int j=0; j<4; j++)
        scores[i][j] += finalScores[j];
    }
  }

  const unsigned totalAlternates = numAlternates;
  const float kScale = 1.0 / totalAlternates;
  float moonProb[13][3];
  float winsTrickProb[13];
  for (unsigned i=0; i<choices.Size(); ++i) {
    int notMoonCount = totalAlternates - (moonCounts[i][0] + moonCounts[i][1]);
    moonProb[i][0] = moonCounts[i][0] * kScale;
    moonProb[i][1] = moonCounts[i][1] * kScale;
    moonProb[i][2] = notMoonCount * kScale;

    winsTrickProb[i] = trickWins[i] * kScale;
  }

  unsigned bestChoice = 0;
  float bestScore = 1e10;
  float expectedScore[choices.Size()];
  for (unsigned i=0; i<choices.Size(); ++i) {
    float score = scores[i][currentPlayer] * kScale;
    expectedScore[i] = score;
    if (bestScore > score) {
      bestScore = score;
      bestChoice = i;
    }
  }

  Card bestPlay = choices.NthCard(bestChoice);

  const AnnotatorPtr annotator = getAnnotator();
  if (annotator)
    annotator->OnWriteData(knowableState, analyzer, expectedScore, moonProb, winsTrickProb);

  delete analyzer;
  return bestPlay;
}
