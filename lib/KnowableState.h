#pragma once

#include "lib/HeartsState.h"
#include "lib/CardArray.h"

#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>
#include <tensorflow/core/framework/tensor.h>

class GameState;
class PossibilityAnalyzer;

namespace tensorflow {
  struct SavedModelBundle;
};

// Doc for Eigen::Tensor is https://bitbucket.org/eigen/eigen/src/de7544f256bdeb135f7d016e2ddf344a9e0406eb/unsupported/Eigen/CXX11/src/Tensor/README.md
typedef Eigen::Tensor<float, 2, Eigen::RowMajor>  FloatMatrix;

// One prediction input is a FloatMatrix with 52 rows and 10 columns
// FloatMatrix predictionInput(52, 10)

enum FeatureColumns
{
  eLegalPlay,
  eCardProbPlayer0,
  eCardProbPlayer1,
  eCardProbPlayer2,
  eCardProbPlayer3,
  eCardPoints,
  eCardOnTable,
  eCardIsHighCardInTrick,
  ePlayerNotRuledOutForMoon,
  eOtherNotRuledOutForMoon,
};

class KnowableState : public HeartsState
{
public:
  static const int kNumFeaturesPerCard = 10;
  static const int kNumFeatures = kNumFeaturesPerCard * kCardsPerDeck;

public:
  KnowableState(const GameState& other);

  GameState HypotheticalState() const;

  PossibilityAnalyzer* Analyze() const;

  CardDeck UnknownCardsForCurrentPlayer() const;

  virtual const CardHand& CurrentPlayersHand() const { return mHand; }

  void PrepareHands(CardHands& hands) const;

  void AsProbabilities(float prob[52][4]) const;
    // Fill the prob array with approximate probabilities of player holding the card.
    // For the current player, we assign 1.0 probability to each card in hand.
    // For the other 3 players, the probabilities are just assigned uniformly across the players who are
    // not void in the card's suit.

  tensorflow::Tensor Transform() const;
    // Transform this state into the the `mainData` tensor input for predict.

  FloatMatrix AsFloatMatrix() const;
    // Returns an Eigen3 maxtrix with kCardsPerDeck rows and kNumFeaturesPerCard columns

  Card Predict(const tensorflow::SavedModelBundle& model, const tensorflow::Tensor& mainData, float playExpectedValue[13]) const;
    // Run tensorflow prediction given the model and tensor input.

  Card TransformAndPredict(const tensorflow::SavedModelBundle& model, float playExpectedValue[13]) const;

  Card ParsePrediction(const std::vector<tensorflow::Tensor>& outputs, float playExpectedValue[13]) const;

private:
  KnowableState();  // unimplemented

  void VerifyKnowableState() const;

  void FillRuledOutForMoonColumnsWhenNoPointsTaken(const CardHand& choices, FloatMatrix& result) const;
  void FillRuledOutForMoonColumnsWhenOtherPlayerRuledOut(const CardHand& choices, FloatMatrix& result) const;
  void FillRuledOutForMoonColumnsWhenCurrentPlayerRuledOut(const CardHand& choices, FloatMatrix& result) const;

private:
  CardHand mHand;
};
