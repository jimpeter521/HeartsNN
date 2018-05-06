#include "lib/KnowableState.h"
#include "lib/GameState.h"
#include "lib/PossibilityAnalyzer.h"

#include "lib/DebugStats.h"

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
  uint128_t possibilityIndex = RandomGenerator::Range128(numPossibilities);

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

float oneIfTrue(bool x) {
  return x ? 1.0 : 0.0;
}

// enum FeatureColumns
// {
//   eLegalPlay,
//   eCardProbPlayer0,
//   eCardProbPlayer1,
//   eCardProbPlayer2,
//   eCardProbPlayer3,
//   eCardPoints,
//   eCardOnTable,
//   eCardIsHighCardInTrick,
//   ePlayerNotRuledOutForMoon,
//   eOtherNotRuledOutForMoon,
// };

FloatMatrix KnowableState::AsFloatMatrix() const
{
  FloatMatrix result(kCardsPerDeck, kNumFeaturesPerCard);
  result.setZero();

  // Fill column eLegalPlay
  const CardHand choices = LegalPlays();
  {
    CardHand::iterator it(choices);
    while (!it.done()) {
      Card card = it.next();
      result(card, eLegalPlay) = 1.0;
    }
  }

  // Fill four columns eCardProbPlayer0 .. eCardProbPlayer3
  float prob[52][4];
  AsProbabilities(prob);
  for (int p=0; p<4; p++) {
    int player = (CurrentPlayer() + p) % 4;
    for (Card card=0; card<52; card++) {
      result(card, eCardProbPlayer0+p) = prob[card][player];
    }
  }

  // Fill column eCardPoints
  for (Card card=0; card<52; ++card) {
  	unsigned pointValue = PointsFor(card);
  	if (pointValue>0 && (UnplayedCards().HasCard(card) || IsCardOnTable(card))) {
    	result(card, eCardPoints) = float(pointValue) / 26.0;
    }
  }

  // Fill column eCardOnTable
  for (int i=0; i<PlayInTrick(); ++i) {
    Card card = GetTrickPlay(i);
    result(card, eCardOnTable) = 1.0;
  }

  // Fill column eCardIsHighCardInTrick
  if (PlayInTrick() > 0)
    result(HighCardOnTable(), eCardIsHighCardInTrick) = 1.0;

  // ePlayerNotRuledOutForMoon and eOtherNotRuledOutForMoon
  // There are several cases to consider, which we handle in separate helper functions
  const unsigned totalPointsTaken = PointsPlayed();
  if (PointsSplit()) {
    // Nothing to do, leave the two feature columns all zeros
  } else if (totalPointsTaken == 0) {
    FillRuledOutForMoonColumnsWhenNoPointsTaken(choices, result);
  } else {
    assert(totalPointsTaken>0 && !PointsSplit());
    const bool currentPlayerCanShoot = GetScoreFor(CurrentPlayer()) == totalPointsTaken;
    if (currentPlayerCanShoot) {
      FillRuledOutForMoonColumnsWhenOtherPlayerRuledOut(choices, result);
    } else {
      FillRuledOutForMoonColumnsWhenCurrentPlayerRuledOut(choices, result);
    }
  }

  return result;
}

void KnowableState::FillRuledOutForMoonColumnsWhenNoPointsTaken(const CardHand& choices, FloatMatrix& result) const
{
  if (PlayInTrick() == 0)
  {
    // No points played, and we are leading.
    CardHand::iterator it(choices);
    while (!it.done())
    {
      Card card = it.next();
      // Since no points are taken, we typically won't be able to lead with a point card,
      // except when we are only holding point cards.
      // If we lead with a non-point card, neither player is ruled out out.
      if (PointsFor(card)==0)
      {
        result(card, ePlayerNotRuledOutForMoon) = 1.0;
        result(card, eOtherNotRuledOutForMoon) = 1.0;
      }
      else
      {
        // If in the rare case that we are forced to lead with a point card,
        // -- we rule the current player out if the play is a point card not guaranteed to take the trick
        // -- we rule others out if we play a point card guaranteed to take the trick
        // But note that the feature vector is "not ruled out", so we have to invert the above logic:
        // -- The current player is not ruled out if we lead with a forcing card
        // -- The other player is not ruled out if we lead with a non-forcing card
        result(card, ePlayerNotRuledOutForMoon) = oneIfTrue(WillCardTakeTrick(card));
        result(card, eOtherNotRuledOutForMoon) = oneIfTrue(!WillCardTakeTrick(card));
      }
    }
  }
  else if (PointsOnTable() > 0)
  {
    CardHand::iterator it(choices);
    while (!it.done()) {
      Card card = it.next();
      // -- we don't rule ourselves out if we play a card that might take the trick.
      result(card, ePlayerNotRuledOutForMoon) = oneIfTrue(MightCardTakeTrick(card));
      // -- we don't rule other players out if we can play a card that doesn't guarantee taking the trick.
      result(card, eOtherNotRuledOutForMoon) = oneIfTrue(!WillCardTakeTrick(card));
    }
  }
  else
  {
    // No points played in either past tricks, or on the table, and we are NOT leading.
    CardHand::iterator it(choices);
    while (!it.done())
    {
      Card card = it.next();
      // If there are no points in play
      // -- No one is ruled out if we play a non-point card
      if (PointsFor(card) == 0) {
        result(card, ePlayerNotRuledOutForMoon) = 1.0;
        result(card, eOtherNotRuledOutForMoon) = 1.0;
      }
      else if (SuitOf(card) == kHearts)
      {
        // -- we rule ourselves out by playing sluffing a heart, but do not rule out others
        result(card, ePlayerNotRuledOutForMoon) = 0.0;
        result(card, eOtherNotRuledOutForMoon) = 1.0;
      }
      else
      {
        assert(card == CardFor(kQueen, kSpades));
        // -- if we play the queen of spades:
        if (TrickSuit() == kSpades)
        {
          // ---- if trick suit is spades:
          // current player is not ruled out if the queen might take the trick
          result(card, ePlayerNotRuledOutForMoon) = oneIfTrue(MightCardTakeTrick(card));
          // other player is not ruled out if the queen is not guranteed to take the trick
          result(card, eOtherNotRuledOutForMoon) = oneIfTrue(!WillCardTakeTrick(card));
        }
        else
        {
          // ---- if trick suit is not spades:
          // By sluffing the queen, the current player is ruled out
          result(card, ePlayerNotRuledOutForMoon) = 0.0;
          // but other player is not yet ruled out
          result(card, eOtherNotRuledOutForMoon) = 1.0;
        }
      }
    }
  }
}

void KnowableState::FillRuledOutForMoonColumnsWhenOtherPlayerRuledOut(const CardHand& choices, FloatMatrix& result) const
{
  assert(PointsPlayed() > 0);
  assert(GetScoreFor(CurrentPlayer()) == PointsPlayed());
  if (PlayInTrick() == 0)
  {
    // The current player is leading.
    CardHand::iterator it(choices);
    while (!it.done())
    {
      Card card = it.next();
      // We don't rule ourself out if we play a non-point card or we play a forcing point card
      result(card, ePlayerNotRuledOutForMoon) = oneIfTrue(PointsFor(card)==0 || WillCardTakeTrick(card));
    }
  }
  else if (PointsOnTable() == 0)
  {
    // The current player is following, and no points are on table
    CardHand::iterator it(choices);
    while (!it.done())
    {
      Card card = it.next();
      // We don't rule ourselves out if we play a non-point card
      result(card, ePlayerNotRuledOutForMoon) = oneIfTrue(PointsFor(card)==0);
    }
  }
  else
  {
    // The current player is following, and points are on table
    CardHand::iterator it(choices);
    while (!it.done())
    {
      Card card = it.next();
      // We don't rule ourselves out if we play a card that might take the trick
      result(card, ePlayerNotRuledOutForMoon) = oneIfTrue(MightCardTakeTrick(card));
    }
  }
}

void KnowableState::FillRuledOutForMoonColumnsWhenCurrentPlayerRuledOut(const CardHand& choices, FloatMatrix& result) const
{
  assert(PointsPlayed() > 0);
  assert(GetScoreFor(CurrentPlayer()) == 0);

  const Card kNoSuchCard = 99u;
  Card otherPlayersPlay = kNoSuchCard;
  for (int p=0; p<PlayInTrick(); p++)
  {
    int player = (PlayerLeadingTrick()+p) % kNumPlayers;
    if (GetScoreFor(player) != 0) {
      otherPlayersPlay = GetTrickPlay(p);
    }
  }

  if (PlayInTrick() == 0)
  {
    assert(otherPlayersPlay == kNoSuchCard);
    // The current player is leading.
    CardHand::iterator it(choices);
    while (!it.done())
    {
      Card card = it.next();
      // We don't rule out the other player if we don't play a forcing card
      result(card, eOtherNotRuledOutForMoon) = oneIfTrue(!WillCardTakeTrick(card));
    }
  }
  else if (PointsOnTable() == 0)
  {
    // The current player is following, and no points are on table
    CardHand::iterator it(choices);
    while (!it.done())
    {
      Card card = it.next();
      // We rule out the other player if we can play a point card and know the other player cannot win the trick.
      // The other player can't win the trick only if they already have a card on the table that can't win the trick.
      result(card, eOtherNotRuledOutForMoon) = oneIfTrue(PointsFor(card)==0 || otherPlayersPlay==kNoSuchCard || !MightCardTakeTrick(card));
    }
  }
  else
  {
    // The current player is following, and points are on table
    CardHand::iterator it(choices);
    while (!it.done())
    {
      Card card = it.next();
      // We can rule out the other player if we can force taking the trick,
      // or if we can tell that the other player already lost the trick.
      if (otherPlayersPlay != kNoSuchCard) {
        // The other player already has their card on the table
        result(card, eOtherNotRuledOutForMoon) = oneIfTrue(otherPlayersPlay==HighCardOnTable() && !MightCardTakeTrick(card));
      } else {
        result(card, eOtherNotRuledOutForMoon) = oneIfTrue(!WillCardTakeTrick(card));
      }
    }
  }
}


tensorflow::Tensor KnowableState::Transform() const
{
  using namespace std;
  using namespace tensorflow;

  float prob[kCardsPerDeck][kNumPlayers];
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

  Tensor mainData(DT_FLOAT, TensorShape({1, kNumFeatures}));
  auto matrix = mainData.matrix<float>();

  // Clear the entire feature vector to all zeros.
  matrix.setZero();

  // Fill the "distribution", the p(hascard | card,player)
  // This must be filled such that the current player is first, then other players in normal order.
  // This fills the first four "feature columns" of 52 elements each.
  int index = 0;
  for (int p=0; p<kNumPlayers; p++) {
    int player = (CurrentPlayer() + p) % 4;
    for (int i=0; i<kCardsPerDeck; i++) {
      matrix(0, index++) = prob[i][player];
    }
  }

  assert(index == kCardsPerDeck*kNumPlayers);

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
  index = kCardsPerDeck*5; // we didn't advance index above, so must set it here.

  // The 6th column is 'high card on table'
  // It is 1 if the card is is on the table and current the high card in the trick suit.
  // This column is all zeros when current player is leading the trick.

  if (PlayInTrick() != 0) {
    matrix(0, index+HighCardOnTable()) = 1.0;
  }
  index = kCardsPerDeck*6; // we didn't advance index above, so must set it here.

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

  assert(index == kCardsPerDeck*7);

  const bool pointsSplit = PointsSplit();
  const unsigned totalPointsTaken = PointsPlayed();

  const bool currentPlayerCanShoot = GetScoreFor(CurrentPlayer()) == totalPointsTaken;

  bool otherPlayerCanShoot = false;
  for (int p=0; p<4; p++) {
    int player = (CurrentPlayer() + p) % 4;
    unsigned pointsForThisPlayer = GetScoreFor(player);
    if (p>0  && pointsForThisPlayer==totalPointsTaken)
      otherPlayerCanShoot = true;
  }

  unsigned pointsOnTheTable = 0;
  for (int p=0; p<4; p++) {
  	Card card = GetTrickPlay(p);
  	pointsOnTheTable += PointsFor(card);
  }

  assert(index == kNumFeatures);

  return mainData;
}

Card KnowableState::TransformAndPredict(const tensorflow::SavedModelBundle& model, float playExpectedValue[13]) const
{
  tensorflow::Tensor mainData = Transform();
  return Predict(model, mainData, playExpectedValue);
}

DebugStats _expectedDeltaPredictionUnclipped("KnowableState::expectedDeltaPredictionUnclipped");
DebugStats _expectedPointsPrediction("KnowableState::expectedPointsPrediction");
DebugStats _expectedScorePrediction("KnowableState::expectedScorePrediction");

Card KnowableState::ParsePrediction(const std::vector<tensorflow::Tensor>& outputs, float playExpectedValue[13]) const
{
  using namespace tensorflow;
  Tensor expectedScorePrediction = outputs.at(0);
  assert(expectedScorePrediction.dims() == 2);
  assert(expectedScorePrediction.NumElements() == kCardsPerDeck);
  auto exectedScoreDelta = expectedScorePrediction.flat<float>();

  Tensor moonProbsPrediction = outputs.at(1);
  assert(moonProbsPrediction.dims() == 3);
  assert(moonProbsPrediction.NumElements() == 3*kCardsPerDeck);
  auto moonProbs = moonProbsPrediction.flat<float>();

  CardHand choices = LegalPlays();

  // playExpectedValue from the NN prediction is for the delta of additional points that the player will take.
  // Below we are only going to use the card with the lowest prediction, but we will clip scores below 0 to 0,
  // so adding the currentScore will actually effect the decision.
  const float kCurrentScore = GetScoreFor(CurrentPlayer());

  // enum MoonCountKey {
  //   kCurrentShotTheMoon = 0,
  //   kOtherShotTheMoon = 1,
  //   kNumMoonCountKeys = 2
  // };

  const float kOffset[3] = { -39.0, 13.0, 0.0 };

  constexpr float kPredictionScoreMax = 26.0;
  constexpr float kModelScoreMax = 2.0;
  constexpr float kScoreScale = kPredictionScoreMax / kModelScoreMax;

  Card bestCard;
  float bestExpected = 1e99;
  CardHand::iterator it(choices);
  for (int i=0; i<choices.Size(); ++i) {
    Card card = it.next();

    float expectedDeltaPrediction = exectedScoreDelta(card);
    _expectedDeltaPredictionUnclipped.Accum(expectedDeltaPrediction);
    const float kMin = 0.0;
    assert(expectedDeltaPrediction >= kMin);
    float expectedPointsPrediction = (expectedDeltaPrediction * kScoreScale) + kCurrentScore;
    expectedPointsPrediction = std::min(kPredictionScoreMax, expectedPointsPrediction);
    _expectedPointsPrediction.Accum(expectedPointsPrediction);

    float expected_score = expectedPointsPrediction - 6.5;

    float check = 0;
    for (int j=0; j<3; j++) {
      const float moon_p = moonProbs(j + card*3);
      assert(moon_p >= 0.0);
      assert(moon_p <= 1.0);
      check += moon_p;

      expected_score += moon_p * kOffset[j];
    }
    assert(abs(check-1.0) < 0.001);

    _expectedScorePrediction.Accum(expected_score);

    playExpectedValue[i] = expected_score;
    if (bestExpected > expected_score) {
      bestExpected = expected_score;
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
