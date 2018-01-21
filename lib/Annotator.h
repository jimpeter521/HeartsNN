// lib/Annotator.h

#pragma once

#include <string>

class GameState;
class KnowableState;
class PossibilityAnalyzer;

class Annotator {
public:
  virtual ~Annotator();
  Annotator();

  virtual void On_DnnMonteCarlo_choosePlay(const KnowableState& state, PossibilityAnalyzer* analyzer
                                 , const float expectedScore[13], const float moonProb[13][3]);

  virtual void OnGameStateBeforePlay(const GameState& state);

  virtual void OnWriteData(const KnowableState& state, PossibilityAnalyzer* analyzer, const float expectedScore[13]
  , const float moonProb[13][3], const float winsTrickProb[13]);
};
