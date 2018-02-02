// lib/WriteDataAnnotator.h
#pragma once

#include "lib/Annotator.h"
#include <stdio.h>

class WriteDataAnnotator : public Annotator {
public:
  ~WriteDataAnnotator();
  WriteDataAnnotator(bool validateMode=false);

  virtual void On_DnnMonteCarlo_choosePlay(const KnowableState& state, PossibilityAnalyzer* analyzer
                                 , const float expectedScore[13], const float moonProb[13][5]);

  virtual void OnGameStateBeforePlay(const GameState& state);

  virtual void OnWriteData(const KnowableState& state, PossibilityAnalyzer* analyzer, const float expectedScore[13]
  , const float moonProb[13][5], const float winsTrickProb[13]);

private:
  const std::string mHash;
  const bool mValidateMode;
  FILE*   mFiles[48];
};
