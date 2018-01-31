#include "lib/Card.h"
#include "lib/GameState.h"
#include "lib/KnowableState.h"
#include "lib/random.h"
#include "lib/RandomStrategy.h"
#include "lib/MonteCarlo.h"
#include "lib/PossibilityAnalyzer.h"
#include "lib/timer.h"

#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <algorithm>

static RandomGenerator rng;

MonteCarlo::~MonteCarlo() {
}

MonteCarlo::MonteCarlo(const StrategyPtr& intuition, const AnnotatorPtr& annotator, uint64_t maxAlternates)
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

Card MonteCarlo::choosePlay(const KnowableState& knowableState) const
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

  // const uint64_t estimatedworkunits =  choices.Size() * (48 - knowableState.PlayNumber());
  double start = now();

  const uint64_t kMinAlternates = 5;
  const double kBudget = 0.333; // For now, a hard-coded budget of a third of a second.

  // For each possible alternate arrangement of opponent's unplayed cards
  unsigned alternate;
  for (alternate=0; alternate<kMaxAlternates; ++alternate)
  {
    const uint128_t possibilityIndex = rng.range128(numPossibilities);

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

    if (alternate>=kMinAlternates && delta(start) > kBudget) {
      // printf("Play %u, choices %u, stopped at %3.2f\n", knowableState.PlayNumber(), choices.Size(), 100.0*float(alternate)/kMaxAlternates);
      break;
    }
  }

  // if (alternate == kMaxAlternates) {
  //   printf("Not stopped: Play %u, choices %u\n", knowableState.PlayNumber(), choices.Size());
  // }

  const unsigned totalAlternates = alternate;
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
