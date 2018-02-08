// lib/DnnMonteCarloAnnotator.h
#pragma once

#include "lib/Annotator.h"

namespace tensorflow {
  struct SavedModelBundle;
};

class DnnMonteCarloAnnotator : public Annotator {
public:
  ~DnnMonteCarloAnnotator();
  DnnMonteCarloAnnotator(const tensorflow::SavedModelBundle& model);

  virtual void On_DnnMonteCarlo_choosePlay(const KnowableState& state, PossibilityAnalyzer* analyzer
                                 , const float expectedScore[13], const float moonProb[13][5]);

  virtual void OnGameStateBeforePlay(const GameState& state);

  virtual void OnWriteData(const KnowableState& state, PossibilityAnalyzer* analyzer, const float expectedScore[13]
  , const float moonProb[13][5], const float winsTrickProb[13]);

private:
  const tensorflow::SavedModelBundle& mModel;
};
