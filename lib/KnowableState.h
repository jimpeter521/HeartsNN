#pragma once

#include "lib/HeartsState.h"
#include "lib/CardArray.h"

#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

class GameState;
class PossibilityAnalyzer;

namespace tensorflow {
  struct SavedModelBundle;
};

class KnowableState : public HeartsState
{
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

  Card Predict(const tensorflow::SavedModelBundle& model, float playExpectedValue[13]) const;

  struct ExtraFeatures {
    float mPlayProgress;
    float mTricksProgess;
    float mInTrickProgress;
    float mGuaranteedSluff[4];
    float mGuaranteedTake[4];
    float mStength[4];
    float mForcedTake[4];
    float mForcedAllow[4];
    float mVulnerability[4];

    float mTotalGuaranteedSluff;
    float mTotalGuaranteedTake;
    float mTotalStrength;
    float mTotalForcedTake;
    float mTotalForcedAllow;
    float mTotalVulnerability;

    void Print(FILE* out);
    void AppendTo(Eigen::TensorMap<Eigen::Tensor<float, 2, 1, long>, 16, Eigen::MakePointer> m, int &index);
  };

  static const int kNumExtraFeatures = sizeof(ExtraFeatures) / sizeof(float);

  void ComputeExtraFeatures(ExtraFeatures& extra) const;

private:
  KnowableState();  // unimplemented

  void VerifyKnowableState() const;

private:

  CardHand mHand;
};
