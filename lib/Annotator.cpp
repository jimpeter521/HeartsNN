// lib/Annotator.cpp

#include "lib/Annotator.h"
#include <assert.h>

Annotator::~Annotator()
{}

Annotator::Annotator()
{}

void Annotator::On_DnnMonteCarlo_choosePlay(const KnowableState& state
                                  , PossibilityAnalyzer* analyzer
                                  , const float expectedScore[13], const float moonProb[13][3])
{
  assert(false);
}

void Annotator::OnGameStateBeforePlay(const GameState& state)
{
  assert(false);
}

void Annotator::OnWriteData(const KnowableState& state, PossibilityAnalyzer* analyzer, const float expectedScore[13]
                          , const float moonProb[13][3], const float winsTrickProb[13])
{
  assert(false);
}
