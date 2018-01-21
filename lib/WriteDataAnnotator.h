// lib/WriteDataAnnotator.h
#pragma once

#include "lib/Annotator.h"
#include <stdio.h>

class WriteDataAnnotator : public Annotator {
public:
  ~WriteDataAnnotator();
  WriteDataAnnotator(const std::string& hash);

  virtual void On_DnnMonteCarlo_choosePlay(const KnowableState& state, PossibilityAnalyzer* analyzer
                                 , const float expectedScore[13], const float moonProb[13][3]);

  virtual void OnGameStateBeforePlay(const GameState& state);

  virtual void OnWriteData(const KnowableState& state, PossibilityAnalyzer* analyzer, const float expectedScore[13]
  , const float moonProb[13][3], const float winsTrickProb[13]);

private:
  const std::string mHash;
  FILE*   mFiles[48];
};
