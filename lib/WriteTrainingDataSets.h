// lib/WriteTrainingDataSets.h
#pragma once

#include "lib/Annotator.h"
#include "lib/NumpyWriter.h"

class WriteTrainingDataSets : public Annotator {
public:
  ~WriteTrainingDataSets();
  WriteTrainingDataSets();

  virtual void On_DnnMonteCarlo_choosePlay(const KnowableState& state, PossibilityAnalyzer* analyzer
                                 , const float expectedScore[13], const float moonProb[13][3]);

  virtual void OnGameStateBeforePlay(const GameState& state);

  virtual void OnWriteData(const KnowableState& state, PossibilityAnalyzer* analyzer, const float expectedScore[13]
  , const float moonProb[13][3], const float winsTrickProb[13]);

private:
  const std::string mHash;
  NumpyWriter<2> mMainDataWriter;
  NumpyWriter<1> mExpectedScoreWriter;
  NumpyWriter<2> mMoonProbWriter;
  NumpyWriter<1> mWinTrickProbWriter;
};
