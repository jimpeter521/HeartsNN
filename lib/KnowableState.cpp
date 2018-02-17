#include "lib/KnowableState.h"
#include "lib/GameState.h"
#include "lib/PossibilityAnalyzer.h"

#include <tensorflow/core/public/session.h>
#include <tensorflow/core/protobuf/meta_graph.pb.h>
#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>
#include <tensorflow/cc/framework/ops.h>

#include <algorithm>

KnowableState::KnowableState(const GameState& gameState)
: HeartsState((const HeartsState&) gameState)
, mHand(gameState.CurrentPlayersHand())
{
  VerifyKnowableState();
}

void KnowableState::VerifyKnowableState() const
{
  assert(mHand.Size() == ((52-(PlayNumber() & ~0x3u)) / 4));
  assert(mHand.AvailableCapacity() == 0);
}

GameState KnowableState::HypotheticalState() const
{
  PossibilityAnalyzer* analyzer = Analyze();
  uint128_t numPossibilities = analyzer->Possibilities();
  uint128_t possibilityIndex = RandomGenerator::gRandomGenerator.range128(numPossibilities);

  CardHands hands;
  PrepareHands(hands);

  analyzer->ActualizePossibility(possibilityIndex, hands);
  return GameState(hands, *this);
}

void KnowableState::PrepareHands(CardHands& hands) const {
  // We need to visit each player's hand, and ensure that the hand is in a valid state to receive
  // cards from the unplayed set of cards.
  // The current player is special -- they just get the exact hand the current player really has.
  // The other three players will each be initialzed to a hand that has no cards, but has capacity
  // to receive the correct number of cards to fill their hand.

  // If we start with the player who led for this trick.

  const unsigned trickStart = PlayNumber() & ~0x3u;    // The play number for start of this trick
  const unsigned maxHolding = (52 - trickStart) / 4;
  assert(mHand.Size() == maxHolding);

  unsigned cardCount = 0;
  for (unsigned i=0; i<4; ++i)                         // i is 0..3, the offset within trick
  {
    const unsigned play = trickStart + i;              // play is the actual play number (0..51) in the game
    const unsigned p = (PlayerLeadingTrick() + i) % 4; // p is the player who played at that point in trick
    if (p == CurrentPlayer()) {
      hands[p] = mHand;                                // p is the current player, they
      cardCount += mHand.Size();
    } else if (play < PlayNumber()) {
      hands[p].PrepForDeal(maxHolding-1);              // p played a card in this trick
      cardCount += hands[p].AvailableCapacity();
    } else {
      assert(play > PlayNumber());
      hands[p].PrepForDeal(maxHolding);
      cardCount += hands[p].AvailableCapacity();
    }
  }

  assert(cardCount == 52 - PlayNumber());
}

PossibilityAnalyzer* KnowableState::Analyze() const
{
  CardDeck remaining = UnplayedCardsNotInHand(mHand);
  unsigned player = CurrentPlayer();

  CardHands hands;
  PrepareHands(hands);

  unsigned capacity = 0;
  for (unsigned i=0; i<4; ++i)
    capacity += hands[i].AvailableCapacity();
  assert(capacity == remaining.Size());

  PriorityList priorityList = MakePriorityList(player, remaining);
  return BuildAnalyzer(player, IsVoidBits(), priorityList, remaining, hands);
}

CardDeck KnowableState::UnknownCardsForCurrentPlayer() const
{
  const CardHand& myHand = CurrentPlayersHand();
  return UnplayedCardsNotInHand(myHand);
}

void KnowableState::AsProbabilities(float prob[52][4]) const
{
  const unsigned current = CurrentPlayer();
  const CardHand unknown = UnknownCardsForCurrentPlayer();

  for (unsigned p=0; p<4; ++p) {
    for (unsigned i=0; i<52; ++i) {
      prob[i][p] = 0.0;
    }
  }

  VoidBits voids =  IsVoidBits().ForOthers(current);

  float suitProb[4] = {0};
  for (Suit suit=0; suit<4; ++suit) {
    unsigned remaining = unknown.CountCardsWithSuit(suit);
    if (remaining == 0) {
      suitProb[suit] = 0.0;
    }
    else {
      unsigned numVoid = voids.CountVoidInSuit(suit);
      assert(numVoid < 3);
      suitProb[suit] = 1.0 / (3-numVoid);
    }
  }

  for (unsigned p=0; p<4; ++p) {
    if (p == current) {
      const CardHand& myHand = CurrentPlayersHand();
      CardHand::iterator it(myHand);
      while (!it.done()) {
        Card card = it.next();
        prob[card][p] = 1.0;
      }
    }
    else {
      const CardHand unknown = UnknownCardsForCurrentPlayer();
      CardHand::iterator it(unknown);
      while (!it.done()) {
        Card card = it.next();
        Suit suit = SuitOf(card);
        if (voids.isVoid(p, suit)) {
          prob[card][p] = 0.0;
        } else {
          prob[card][p] = suitProb[suit];
        }
      }
    }
  }
}

static int CountCardsLowerThan(const CardArray& cards, Card sentinel) {
  const uint64_t mask = (1ul << sentinel) - 1;
  return cards.CountCardsWithMask(mask);
}

static int CountCardsHigherThan(const CardArray& cards, Card sentinel) {
  const uint64_t mask = (1ul << sentinel) - 1;
  return cards.CountCardsWithMask(~mask);
}

void KnowableState::ComputeExtraFeatures(ExtraFeatures& extra) const
{
  const int kPlayNumber = PlayNumber();
  const int kTrickNumber = kPlayNumber / 4;
  const int kPlayInTrick = PlayInTrick();

  // The following three features may be pointless, but it may be useful to give the NN a more direct sense
  // of game and trick "time" than it can infer from other information.
  // All three are in the range [0.0, 1.0]

  // The last possible play where a player may have a choice is play #47.
  extra.mPlayProgress = kPlayNumber / 47.0;

  // The last trick is forced (#12, counting from zero), so we don't count it.
  extra.mTricksProgess = kTrickNumber / 11.0;

  // The last play in a trick is #3 (counting from zero)
  extra.mInTrickProgress = kPlayInTrick / 3.0;

  CardDeck remaining = UnplayedCardsNotInHand(mHand);

  // 24 features in this loop
  for (Suit suit=0; suit<kSuitsPerDeck; ++suit) {
    const CardArray unplayedInSuit = remaining.CardsWithSuit(suit);
    const CardArray myHandInSuit = mHand.CardsWithSuit(suit);

    int guaranteedSluff = 0;
    int guaranteedTake = 0;
    if (unplayedInSuit.Size() == 0) {
      // If the other players are all void in the suit, then all of my cards are guaranteed takes & sluffs
      guaranteedSluff = guaranteedTake = myHandInSuit.Size();
    } else {
      Card lowestUnplayed = unplayedInSuit.FirstCard();
      Card highestUnplayed = unplayedInSuit.LastCard();

      // For each suit, number of cards in hand that are lower than any unplayed cards of the suit (guaranteed sluffs)
      guaranteedSluff = CountCardsLowerThan(myHandInSuit, lowestUnplayed);

      // For each suit, number of cards in hand that are higher than any unplayed cards of the suit (guaranteed takes)
      guaranteedTake = CountCardsHigherThan(myHandInSuit, highestUnplayed);
    }

    // Normalize to size of hand, so that values are scaled similarly throughout the game
    extra.mGuaranteedSluff[suit] = float(guaranteedSluff) / mHand.Size();
    extra.mGuaranteedTake[suit] = float(guaranteedTake) / mHand.Size();
    // For each suit, the min of these two numbers. When greater than zero, the suit is strong and flexible
    extra.mStength[suit] = std::min(extra.mGuaranteedSluff[suit], extra.mGuaranteedTake[suit]);

    int forcedTake = 0;
    int forcedAllows = 0;
    if (myHandInSuit.Size() > 0) {
      const Card lowestUnplayed = myHandInSuit.FirstCard();
      const Card highestUnplayed = myHandInSuit.LastCard();

      // For each suit, number of unplayed cards lower than my lowest card of the suit (a crude measure of forced takes)
      forcedTake = CountCardsLowerThan(unplayedInSuit, lowestUnplayed);

      // For each suit, number of unplayed cards that are higher than my highest cards of the suit (a crude measure of forced allows)
      forcedAllows = CountCardsHigherThan(unplayedInSuit, highestUnplayed);
    }
    // Normalize to number of unplayed cards, so that values are scaled similarly throughout the game
    extra.mForcedTake[suit] = float(forcedTake) / remaining.Size();
    extra.mForcedAllow[suit] = float(forcedAllows) / remaining.Size();
    // For each suit, the min of these two numbers. When greater than zero, the suit is weak, i.e has little control.
    extra.mVulnerability[suit] = std::min(extra.mForcedTake[suit], extra.mForcedAllow[suit]);
  }
}


static float normalizeScore(float s) {
  return s * 19.5;
}

float oneIfTrue(bool x) {
  return x ? 1.0 : 0.0;
}

tensorflow::Tensor KnowableState::Transform() const
{
  using namespace std;
  using namespace tensorflow;

  float prob[52][4];
  #if 1
    AsProbabilities(prob);
  #else
    PossibilityAnalyzer* analyzer = state.Analyze();
    const uint128_t numPossibilities = analyzer->Possibilities();

    Distribution distribution;
    CardHands hands;
    state.PrepareHands(hands);
    analyzer->ExpectedDistribution(distribution, hands);
    distribution.DistributeRemainingToPlayer(state.CurrentPlayersHand(), state.CurrentPlayer(), numPossibilities);

    delete analyzer;
    distribution.AsProbabilities(prob);
  #endif

  assert(kNumExtraFeatures == 33);

  const int kNumPlayers = 4;
  const int kNumFeaturesPerCard = kNumPlayers + 3;
  const int kMoonFlagsLen = 4;
  const int kPlaysPerTrick = 4;
	const int kPointsExtraFeatures = kPlaysPerTrick + 1 + kMoonFlagsLen;

  const int kNumFeatures = kNumFeaturesPerCard * kCardsPerDeck + kPointsExtraFeatures + kNumExtraFeatures;

  Tensor mainData(DT_FLOAT, TensorShape({1, kNumFeatures}));
  auto matrix = mainData.matrix<float>();

  // Clear the entire feature vector to all zeros.
  int index = 0;
  for (; index<kNumFeatures; index++) {
    matrix(0, index) = 0;
  }

  // Fill the "distribution", the p(hascard | card,player)
  // This must be filled such that the current player is first, then other players in normal order.
  // This fills the first four "feature columns" of 52 elements each.
  index = 0;
  for (int p=0; p<4; p++) {
    int player = (CurrentPlayer() + p) % 4;
    for (int i=0; i<52; i++) {
      matrix(0, index++) = prob[i][player];
    }
  }

  assert(index == 52*4);

  const CardHand choices = LegalPlays();

  // The 5th column is legal plays
  // Set to 1 just the elements corresponding to the cards that are legal plays

  {
    CardHand::iterator it(choices);
    while (!it.done()) {
      Card card = it.next();
      matrix(0, index+card) = 1.0;
    }
  }
  index = 52*5; // we didn't advance index above, so must set it here.

  // The 6th column is 'high card on table'
  // It is 1 if the card is is on the table and current the high card in the trick suit.
  // This column is all zeros when current player is leading the trick.

  if (PlayInTrick() != 0) {
    matrix(0, index+HighCardOnTable()) = 1.0;
  }
  index = 52*6; // we didn't advance index above, so must set it here.

  // The last 52-elem feature column is the number of points each unplayed card is worth.
  // Cards already played score 0 here, and of course non-point cards are also 0.
  // We scale by 1/26.0 so the points here are scaled the same way as elsewhere.
  // We hope that the DNN will learn that points flow from here, to the points on table,
  // and then into the points taken so far.
  for (Card card=0; card<52; ++card) {
  	unsigned pointValue = PointsFor(card);
  	if (pointValue>0 && UnplayedCards().HasCard(card)) {
    	matrix(0, index++) = float(pointValue) / 26.0;
    } else {
     	matrix(0, index++) = 0.0;
   }
  }

  assert(index == 52*7);

  // The following code fills 9 slots in the feature vector. These 8 features are "extra" features,
  // but they are not currently part of the `ExtraFeatures` struct.
  // TODO: Consolidate them into the ExtraFeatures struct.

  const unsigned totalPointsTaken = PointsPlayed();
  const bool pointsBroken = totalPointsTaken > 0;
  const bool pointsSplit = PointsSplit();
  if (!pointsBroken) {
    assert(!pointsSplit);
  }
  if (pointsSplit) {
    assert(pointsBroken);
  }

  const bool currentPlayerCanShoot = GetScoreFor(CurrentPlayer()) == totalPointsTaken;

  bool otherPlayerCanShoot = false;
  for (int p=0; p<4; p++) {
    int player = (CurrentPlayer() + p) % 4;
    unsigned pointsForThisPlayer = GetScoreFor(player);
    if (p>0  && pointsForThisPlayer==totalPointsTaken)
      otherPlayerCanShoot = true;
    matrix(0, index++) = GetScoreFor(player) / 26.0;    // Points so far for the 4 players, rotated so current player is first.
  }

  unsigned pointsOnTheTable = 0;
  for (int p=0; p<4; p++) {
  	Card card = GetTrickPlay(p);
  	pointsOnTheTable += PointsFor(card);
  }
  matrix(0, index++) = pointsOnTheTable / 26.0;    // The scaled points face up on the table.

  if (!pointsBroken) {
    assert(!pointsSplit);
  } else if (pointsSplit) {
    assert(pointsBroken);
    assert(!currentPlayerCanShoot);
    assert(!otherPlayerCanShoot);
  } else if (!pointsSplit) {
    assert(pointsBroken);
    assert(otherPlayerCanShoot != currentPlayerCanShoot);
    assert(otherPlayerCanShoot || currentPlayerCanShoot);
  }

  matrix(0, index++) = oneIfTrue(!pointsSplit);
  matrix(0, index++) = oneIfTrue(pointsSplit);
  matrix(0, index++) = oneIfTrue(currentPlayerCanShoot);
  matrix(0, index++) = oneIfTrue(otherPlayerCanShoot);

  assert(index == kNumFeatures - kNumExtraFeatures);

  ExtraFeatures extra_features;
  ComputeExtraFeatures(extra_features);

  extra_features.AppendTo(matrix, index);

  assert(index == kNumFeatures);

  return mainData;
}

Card KnowableState::TransformAndPredict(const tensorflow::SavedModelBundle& model, float playExpectedValue[13]) const
{
  tensorflow::Tensor mainData = Transform();
  return Predict(model, mainData, playExpectedValue);
}

Card KnowableState::ParsePrediction(const std::vector<tensorflow::Tensor>& outputs, float playExpectedValue[13]) const
{
  using namespace tensorflow;
  Tensor prediction = outputs.at(0);
  assert(prediction.dims() == 2);
  assert(prediction.NumElements() == 52);
  auto vec = prediction.flat<float>();

  CardHand choices = LegalPlays();

  Card bestCard;
  float bestExpected = 1e99;
  CardHand::iterator it(choices);
  for (int i=0; i<choices.Size(); ++i) {
    Card card = it.next();
    float expected = normalizeScore(vec(card));
    playExpectedValue[i] = expected;
    if (bestExpected > expected) {
      bestExpected = expected;
      bestCard = card;
    }
  }

  return bestCard;
}

Card KnowableState::Predict(const tensorflow::SavedModelBundle& model, const tensorflow::Tensor& mainData, float playExpectedValue[13]) const
{
  using namespace std;
  using namespace tensorflow;

  std::vector<Tensor> outputs;
  auto result = model.session->Run({{"main_data:0", mainData}}, {"expected_score/expected_score:0"}, {}, &outputs);
  if (!result.ok()) {
    printf("Tensorflow prediction failed: %s\n", result.error_message().c_str());
    exit(1);
  }

  return ParsePrediction(outputs, playExpectedValue);
}

static void Print5(FILE* out, float data[4]) {
  float total = data[0] + data[1] + data[2] + data[3];
  fprintf(out, "%3.2f %3.2f %3.2f %3.2f %3.2f\n", data[0], data[1], data[2], data[3], total);
}

void KnowableState::ExtraFeatures::Print(FILE* out) {
  fprintf(out, "%3.2f %3.2f %3.2f\n", mPlayProgress, mTricksProgess, mInTrickProgress);
  Print5(out, mGuaranteedSluff);
  Print5(out, mGuaranteedTake);
  Print5(out, mStength);
  Print5(out, mForcedTake);
  Print5(out, mForcedAllow);
  Print5(out, mVulnerability);
}

static void Append5(Eigen::TensorMap<Eigen::Tensor<float, 2, 1, long>, 16, Eigen::MakePointer> m, int &index, float f[4])
{
  float total = 0;
  for (int i=0; i<4; ++i) {
    total += f[i];
    m(0, index++) = f[i];
  }
  m(0, index++) = total;
}

void KnowableState::ExtraFeatures::AppendTo(Eigen::TensorMap<Eigen::Tensor<float, 2, 1, long>, 16, Eigen::MakePointer> m, int &index)
{
  m(0, index++) = mPlayProgress;
  m(0, index++) = mTricksProgess;
  m(0, index++) = mInTrickProgress;
  Append5(m, index, mGuaranteedSluff);
  Append5(m, index, mGuaranteedTake);
  Append5(m, index, mStength);
  Append5(m, index, mForcedTake);
  Append5(m, index, mForcedAllow);
  Append5(m, index, mVulnerability);
}
