#include "lib/DnnMonteCarloAnnotator.h"
#include "lib/GameState.h"
#include "lib/KnowableState.h"
#include "lib/PossibilityAnalyzer.h"
#include <stdio.h>

DnnMonteCarloAnnotator::~DnnMonteCarloAnnotator()
{

}

DnnMonteCarloAnnotator::DnnMonteCarloAnnotator(const tensorflow::SavedModelBundle& model)
: mModel(model)
{
}

void DnnMonteCarloAnnotator::On_DnnMonteCarlo_choosePlay(const KnowableState& state, PossibilityAnalyzer* analyzer
                               , const float empiricalExpectedScore[13], const float empiricalMoonProb[13][3])
{
  const CardHand choices = state.LegalPlays();
  printf("Play %d, Player Leading %d, Current Player %d, Choices %d TrickSuit %s\n"
      , state.PlayNumber(), state.PlayerLeadingTrick(), state.CurrentPlayer(), choices.Size(), NameOfSuit(state.TrickSuit()));

  if (state.PlayInTrick() != 0)
    assert(SuitOf(state.GetTrickPlay(0)) == state.TrickSuit());
  printf("TrickSoFar:"); // no \n
  for (int i=0; i<4; i++) {
    if (i < state.PlayInTrick())
      printf("%3s  ", NameOf(state.GetTrickPlay(i)));
    else
      printf(". ");
  }
  printf("\n");

  printf("PointsSoFar:");  // no \n
  for (int i=0; i<4; i++) {
    printf("%d ", state.GetScoreFor(i));
  }
  printf("\n");

  const uint128_t kPossibilities = analyzer->Possibilities();
  Distribution distribution;
  CardHands hands;
  state.PrepareHands(hands);
  analyzer->ExpectedDistribution(distribution, hands);
  distribution.DistributeRemainingToPlayer(state.CurrentPlayersHand(), state.CurrentPlayer(), kPossibilities);
  distribution.Validate(state.UnplayedCards(), kPossibilities);

  float prob[52][4];
  distribution.AsProbabilities(prob);
  Distribution::PrintProbabilities(prob, stdout);

  float predictedExpectedScore[13];
  state.TransformAndPredict(mModel, predictedExpectedScore);

  CardArray::iterator it(choices);
  for (unsigned i=0; i<choices.Size(); ++i) {
    Card card = it.next();
    printf("%3s em=%4.2f pr=%4.2f | ", NameOf(card), empiricalExpectedScore[i], predictedExpectedScore[i]);
    for (int j=0; j<3; j++)
      printf(" em=%2.1f", empiricalMoonProb[i][j]);
    printf("\n");
  }
  printf("----\n");
}

void DnnMonteCarloAnnotator::OnGameStateBeforePlay(const GameState& state)
{
  state.PrintState();
}

void DnnMonteCarloAnnotator::OnWriteData(const KnowableState& state, PossibilityAnalyzer* analyzer, const float empiricalExpectedScore[13]
                          , const float empiricalMoonProb[13][3], const float winsTrickProb[13])
{
}
