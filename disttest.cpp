#include "lib/GameState.h"
#include "lib/KnowableState.h"
#include "lib/math.h"
#include "lib/PossibilityAnalyzer.h"
#include "lib/RandomStrategy.h"
#include "lib/MonteCarlo.h"

#include <iostream>
#include <fstream>
#include <sstream>

std::string basePath("/Users/jim/dev/github/deck/dot/d");

static std::string debugFileName(int playNumber) {
  const std::string pad = playNumber<10 ? "0" : "";
  return basePath + pad + std::to_string(playNumber) + ".dot";
}

const uint32_t kMinAlternates = 30;
const uint32_t kMaxAlternates = 2000;
const float kTimeBudget = 0.25;

int run()
{
  StrategyPtr intuition(new RandomStrategy());
  StrategyPtr monte(new MonteCarlo(intuition, kMinAlternates, kMaxAlternates, kTimeBudget));

  GameState state;
  PossibilityAnalyzer* analyzer = 0;

  const uint128_t kThresh = 10000;

  uint128_t possibilities = 0;
  while (!state.Done())
  {
    KnowableState knowableState(state);

    analyzer = knowableState.Analyze();
    possibilities = analyzer->Possibilities();

    std::string poss = asDecimalString(possibilities);

    if (possibilities < kThresh) {
      printf("At play %d, possibilities, %s\n", state.PlayNumber(), poss.c_str());
      break;
    }

    Card card = monte->choosePlay(knowableState);
    state.PlayCard(card);
  }

  Distribution analyzedDistribution;
  {
    KnowableState knowableState(state);
    CardHands hands;
    knowableState.PrepareHands(hands);
    analyzer->ExpectedDistribution(analyzedDistribution, hands);
    analyzedDistribution.DistributeRemainingToPlayer(state.CurrentPlayersHand(), state.CurrentPlayer(), possibilities);
  }

  Distribution distribution;

  for (uint128_t possibility=0; possibility<possibilities; ++possibility) {
    KnowableState knowableState(state);
    CardHands hands;
    knowableState.PrepareHands(hands);
    analyzer->ActualizePossibility(possibility, hands);
    distribution.CountOccurrences(hands);
  }


  if (analyzedDistribution == distribution)
  {
    // distribution.Print();

    // float prob[52][4];
    // distribution.AsProbabilities(prob);
    // Distribution::PrintProbabilities(prob);

    return 0;
  }
  else
  {
    KnowableState knowableState(state);
    int p = state.PlayNumber();
    analyzer->RenderDotToFile(debugFileName(p), knowableState.UnknownCardsForCurrentPlayer());

    for (int player=0; player<4; player++) {
      const char X = player == state.CurrentPlayer() ? '*' : ' ';
      putchar(X);
      state.PrintHand(player);
    }
    // printf("Analyzed distribution\n");
    // analyzedDistribution.Print();
    // printf("\nOccurences distribution\n");
    // distribution.Print();
    return 1;
  }

}

int main(int argc, char** argv)
{
  const int kRuns = argc==2 ? atoi(argv[1]) : 2;
  assert(kRuns > 0);
  assert(kRuns < 1000000);
  for (int i=0; i<kRuns; i++) {
    int result = run();
    if (result)
      return result;
    else {
      printf("Ok %d of %d\n", i, kRuns);
    }
  }
  return 0;
}
