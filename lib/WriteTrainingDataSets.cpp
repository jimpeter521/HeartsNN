// lib/WriteTrainingDataSets.cpp

#include "lib/WriteTrainingDataSets.h"
#include "lib/GameState.h"
#include "lib/KnowableState.h"
#include "lib/PossibilityAnalyzer.h"
#include "lib/random.h"

#include <assert.h>
#include <sys/stat.h>

WriteTrainingDataSets::~WriteTrainingDataSets()
{
}

const std::string dataDirPath("data/");

WriteTrainingDataSets::WriteTrainingDataSets()
: mHash(asHexString(RandomGenerator::Random128()))
, mMainDataWriter(dataDirPath+mHash+"-main.npy", std::vector<int>({52, 10}))
, mExpectedScoreWriter(dataDirPath+mHash+"-score.npy", std::vector<int>({52}))
, mMoonProbWriter(dataDirPath+mHash+"-moon.npy", std::vector<int>({52, 3}))
, mWinTrickProbWriter(dataDirPath+mHash+"-trick.npy", std::vector<int>({52}))
{
}

void WriteTrainingDataSets::On_DnnMonteCarlo_choosePlay(const KnowableState& state
                                  , PossibilityAnalyzer* analyzer
                                  , const float expectedScore[13], const float moonProb[13][3])
{
}

void WriteTrainingDataSets::OnGameStateBeforePlay(const GameState& state)
{
}

void WriteTrainingDataSets::OnWriteData(const KnowableState& state, PossibilityAnalyzer* analyzer, const float expectedScore[13]
                          , const float moonProb[13][3], const float winsTrickProb[13])
{
  FloatMatrix mainData = state.AsFloatMatrix();
  mMainDataWriter.Append(mainData);

  const CardHand choices = state.LegalPlays();

  FloatVector scoreData(kCardsPerDeck);
  scoreData.setZero();
  FloatVector trickData(kCardsPerDeck);
  trickData.setZero();
  FloatMatrix moonData(kCardsPerDeck, 3);
  moonData.setZero();

  CardHand::iterator it(choices);
  int i = 0;
  while (!it.done()) {
    Card card = it.next();
    scoreData(card) = expectedScore[i];
    trickData(card) = winsTrickProb[i];

    for (int j=0; j<3; ++j) {
      moonData(card, j) = moonProb[i][j];
    }
    ++i;
  }

  mExpectedScoreWriter.Append(scoreData);
  mMoonProbWriter.Append(moonData);
  mWinTrickProbWriter.Append(trickData);
}
