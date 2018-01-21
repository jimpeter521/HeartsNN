#include "lib/Card.h"
#include "lib/GameState.h"
#include "lib/KnowableState.h"
#include "lib/random.h"
#include "lib/DnnModelStrategy.h"
#include "lib/DnnMonteCarlo.h"
#include "lib/PossibilityAnalyzer.h"
#include "lib/WriteDataAnnotator.h"
#include "lib/math.h"
#include "lib/timer.h"

#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <algorithm>

static RandomGenerator rng;

DnnMonteCarlo::~DnnMonteCarlo() {
  delete mWriteDataAnnotator;
}

DnnMonteCarlo::DnnMonteCarlo(const tensorflow::SavedModelBundle& model, bool writeData)
: Strategy()
, mHash(asHexString(rng.random128()))
, mRolloutStrategy(new DnnModelStrategy(model))
, mWriteDataAnnotator(0)
{
  assert(mRolloutStrategy != 0);
  if (writeData)
    mWriteDataAnnotator = new WriteDataAnnotator(asHexString(rng.random128(), 32));
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

Card DnnMonteCarlo::choosePlay(const KnowableState& knowableState, Annotator* annotator) const
{
  // knowableState.VerifyHeartsState();
  // double start = now();

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

  double begin = now();

  const unsigned kMinAlternates = 20;
  const unsigned kMaxAlternates = 40;

  const double kTimeLimit = 0.2;
  unsigned numAlternates = 0;
  while (numAlternates<kMinAlternates || !(numAlternates>=kMaxAlternates || delta(begin) > kTimeLimit))
  {
    ++numAlternates;
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

      float finalScores[4] = {0};

      bool shotTheMoon;
      next.PlayOutGameMonteCarlo(finalScores, shotTheMoon, mRolloutStrategy);

      next.TrackTrickWinner(0);
      if (shotTheMoon)
        updateMoonStats(currentPlayer, i, finalScores, moonCounts);

      for (int j=0; j<4; j++)
        scores[i][j] += finalScores[j];
    }
  }

  // printf("play: %d, numAlternates: %d\n", knowableState.PlayNumber(), numAlternates);

  const float kScale = 1.0 / numAlternates;
  float moonProb[13][3];
  float winsTrickProb[13];
  for (unsigned i=0; i<choices.Size(); ++i) {
    int notMoonCount = numAlternates - (moonCounts[i][0] + moonCounts[i][1]);
    moonProb[i][0] = moonCounts[i][0] * kScale;
    moonProb[i][1] = moonCounts[i][1] * kScale;
    moonProb[i][2] = notMoonCount * kScale;

    winsTrickProb[i] = trickWins[i] * kScale;
  }

  float expectedScore[choices.Size()];
  for (unsigned i=0; i<choices.Size(); ++i) {
    expectedScore[i] = scores[i][currentPlayer] * kScale;
  }

  double bestScore = 1e10;
  Card bestPlay = choices.FirstCard();
  CardArray::iterator it(choices);
  for (unsigned i=0; i<choices.Size(); ++i)
  {
    Card card = it.next();
    if (bestScore > scores[i][currentPlayer])
    {
      bestScore = scores[i][currentPlayer];
      bestPlay = card;
    }
  }

  if (annotator != 0) {
    annotator->On_DnnMonteCarlo_choosePlay(knowableState, analyzer, expectedScore, moonProb);
  }

  if (mWriteDataAnnotator)
    mWriteDataAnnotator->OnWriteData(knowableState, analyzer, expectedScore, moonProb, winsTrickProb);

  delete analyzer;

  // double duration = delta(start);
  // printf("One DnnMonteCarlo::choosePlay: %4.3fsecs\n", duration);

  return bestPlay;
}
