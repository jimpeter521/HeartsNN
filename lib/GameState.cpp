#include "lib/GameState.h"
#include "lib/Annotator.h"
#include "lib/KnowableState.h"
#include "lib/RandomStrategy.h"

#include <assert.h>

GameState::GameState()
    : HeartsState(Deal::RandomDealIndex())
    , mHands()
{
  Deal deck(dealIndex());

  for (int i = 0; i < 4; i++)
  {
    mHands[i] = deck.dealFor(i);
  }
  SetLead(deck.startPlayer());
  assert(mHands[PlayerLeadingTrick()].FirstCard() == 0);

  VerifyGameState();
}

GameState::GameState(const Deal& deck)
    : HeartsState(deck.dealIndex())
    , mHands()
{
  for (int i = 0; i < 4; i++)
  {
    mHands[i] = deck.dealFor(i);
  }
  SetLead(deck.startPlayer());
  assert(mHands[PlayerLeadingTrick()].FirstCard() == 0);

  VerifyGameState();
}

GameState::GameState(const GameState& other)
    : HeartsState(other)
{
  for (int i = 0; i < 4; i++)
  {
    mHands[i] = other.mHands[i];
  }
  VerifyGameState();
}

GameState::GameState(const GameState& other, const CardHands& hands)
    : HeartsState(other)
{
  other.IsVoidBits().VerifyVoids(hands);

  const int current = CurrentPlayer();
  for (int i = 0; i < 4; i++)
  {
    if (i == current)
    {
      assert(other.mHands[i] == hands[i]);
    }
    else
    {
      assert(other.mHands[i].Size() == hands[i].Size());
    }
    mHands[i] = hands[i];
  }
  VerifyGameState();
}

GameState::GameState(const CardHands& hands, const KnowableState& knowableState)
    : HeartsState(knowableState)
{
  knowableState.IsVoidBits().VerifyVoids(hands);
  for (int i = 0; i < 4; i++)
    mHands[i] = hands[i];
  VerifyGameState();
}

void GameState::VerifyGameState() const
{
#ifndef NDEBUG
  assert(CurrentPlayersHand().Size() == ((52 - (PlayNumber() & ~0x3u)) / 4));
  assert(CurrentPlayersHand().AvailableCapacity() == 0);

  IsVoidBits().VerifyVoids(mHands);
#endif
}

GameOutcome GameState::PlayGame(StrategyPtr players[4], const RandomGenerator& rng)
{
  // We allow calling this method from a GameState in the middle of the game.
  // so we intentionally do not set mNextPlay=0 for first iteration of loop here.
  while (!Done())
  {
    int i = CurrentPlayer();
    StrategyPtr player = players[i];
    AnnotatorPtr annotator = player->getAnnotator();
    if (annotator)
    {
      annotator->OnGameStateBeforePlay(*this);
    }
    NextPlay(player, rng);
  }
  return CheckForShootTheMoon();
}

// This is how we do one "rollout" to the end of the game.
// The strategy passed in here should be an "intuition" strategy,
// either the RandomStrategy or the DnnModelIntuition strategy.
GameOutcome GameState::PlayOutGameMonteCarlo(const StrategyPtr& opponent, const RandomGenerator& rng)
{
  while (!Done())
  {
    NextPlay(opponent, rng);
  }
  return CheckForShootTheMoon();
}

void GameState::PrintPlay(int player, Card card) const
{
  const char* name = NameOf(card);
  printf("%d %s | ", player, name);
  mHands[player].Print();

  if (PlayInTrick() == 0)
    printf("----\n");
}

void GameState::PrintState() const
{
  printf("Play %d, Player %d has the lead\n", PlayNumber(), PlayerLeadingTrick());
  const int playInTrick = PlayInTrick();
  for (int i = 0; i < 4; i++)
  {
    int p = (PlayerLeadingTrick() + i) % 4;

    if (i < playInTrick)
    {
      const char* name = NameOf(GetTrickPlay(i));
      printf("%s |", name);
    }
    mHands[p].Print();
  }
}

void GameState::PlayCard(Card card)
{
  const int currentPlayer = CurrentPlayer();
  const CardHand hand = CurrentPlayersHand();
  assert(LegalPlays().HasCard(card));
  assert(hand.HasCard(card));

  const int play = PlayNumber();

  if (mPlayCardHook)
    (mPlayCardHook)(play, currentPlayer, card);

  const int playInTrick = PlayInTrick();

  SetTrickPlay(playInTrick, card);
  RemoveUnplayedCard(card);
  mHands[currentPlayer].RemoveCard(card);

  // If this is the first card in the trick, it determines the suit for the trick
  if (play == 0)
  {
    assert(TrickSuit() == kUnknown);
    assert(card == CardFor(kTwo, kClubs));
    SetTrickSuit(kClubs);
  }
  else if (playInTrick == 0)
  {
    assert(TrickSuit() == kUnknown);
    SetTrickSuit(SuitOf(card));
  }
  else if (TrickSuit() != SuitOf(card))
  {
    assert(mHands[currentPlayer].CountCardsWithSuit(TrickSuit()) == 0);
    setIsVoid(currentPlayer, TrickSuit());
  }

  // If this is the last card in the trick
  if (playInTrick == 3)
  {
    const int winner = TrickWinner();
    const unsigned pointsInTrick = ScoreTrick();
    int lead = NewLead(winner);
    AddToScoreFor(lead, pointsInTrick);
    if (mTrickResultHook)
      (mTrickResultHook)(lead, PointsSoFar());
  }

  // Only now do we advance the play number.
  AdvancePlayNumber();
}

Card GameState::NextPlay(const StrategyPtr& currentPlayersStrategy, const RandomGenerator& rng)
{
  Card card;
  const CardHand choices = LegalPlays();
  if (PointsPlayed() == 26 || choices.Size() == 1)
  {
    card = choices.FirstCard();
  }
  else
  {
    KnowableState knowableState(*this);
    card = currentPlayersStrategy->choosePlay(knowableState, rng);
    assert(choices.HasCard(card));
  }
  PlayCard(card);
  return card;
}

visible::VisibleState GameState::asVisibleState(unsigned player) const
{
  KnowableState knowable{*this};
  auto builder = visible::VisibleState::Builder();
  builder.add(knowable.CurrentPlayersHand());

  unsigned p = knowable.PlayerLeadingTrick();
  unsigned numOnTable = knowable.PlayInTrick();
  for (auto i=0; i<numOnTable; ++i)
  {
    Card card = knowable.GetTrickPlay(i);
    builder.played(p, card);
    p = (p+1) % 4;
  }

  return builder.build();
}
