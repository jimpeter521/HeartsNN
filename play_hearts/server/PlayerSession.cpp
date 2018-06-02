// play_hearts/PlayerSession.cpp

#include "play_hearts/server/PlayerSession.h"

#include "lib/random.h"
#include "play_hearts/conversions.h"
#include "play_hearts/server/ClientPlayer.h"

#include <algorithm>

using playhearts::Hello;

PlayerSession::PlayerSession(ServerReaderWriter<ServerMessage, ClientMessage>* stream, const char* modelpath)
    : mStream(stream)
    , mModelPath(modelpath)
{
  assert(mModelPath != nullptr);
  mTotals.fill(0);
  mReferenceTotals.fill(0);
}

Status PlayerSession::ManageSession()
{
  ClientMessage clientMessage;
  while (mStream->Read(&clientMessage))
  {
    if (clientMessage.req_case() != ClientMessage::kPlayer)
      assert(clientMessage.sessiontoken() == mSessionToken);

    switch (clientMessage.req_case())
    {
      case ClientMessage::kPlayer:
      {
        OnPlayer(clientMessage.player());
        break;
      }
      case ClientMessage::kStartGame:
      {
        OnStartGame(clientMessage.startgame());
        break;
      }
      case ClientMessage::kMyPlay:
      {
        assert(false); // this is now handed in ClientPlayer::choosePlay()
        // OnMyPlay(clientMessage.myplay());
        break;
      }
      case ClientMessage::REQ_NOT_SET:
      {
        assert(false);
        break;
      }
    }
  }

  std::cout << "Server Connect ending." << std::endl;
  return Status::OK;
}

void PlayerSession::OnPlayer(const Player& player)
{
  mPlayerName = player.name();
  mPlayerEmail = player.email();

  std::cout << "Received Player " << mPlayerName << " " << mSessionToken << std::endl;

  uint128_t N = RandomGenerator::Random128();
  mSessionToken = asHexString(N);

  ServerMessage serverMessage;
  Hello* helloMessage = serverMessage.mutable_hello();
  helloMessage->set_sessiontoken(mSessionToken);
  mStream->Write(serverMessage);
  std::cout << "Sent Hello " << mSessionToken << std::endl;
}

void PlayerSession::OnStartGame(const StartGame& startGame)
{
  std::cout << "Received StartGame " << mPlayerName << " " << mSessionToken << std::endl;

  uint128_t N = Deal::RandomDealIndex();
  GameState gameState(N);

  gameState.SetPlayCardHook([this, &gameState](int play, int player, Card card) {
    ServerMessage serverMessage;
    playhearts::CardPlayed* cardPlayed = serverMessage.mutable_cardplayed();
    cardPlayed->set_playnumber(gameState.PlayNumber());
    cardPlayed->set_player(gameState.CurrentPlayer());
    setProtocolCard(cardPlayed->mutable_card(), card);
    this->mStream->Write(serverMessage);
  });

  gameState.SetTrickResultHook([this](int trickWinner, const std::array<unsigned, 4>& points) {
    ServerMessage serverMessage;
    playhearts::TrickResult* trickResult = serverMessage.mutable_trickresult();
    trickResult->set_trickwinner(trickWinner);
    for (int i = 0; i < 4; i++)
    {
      trickResult->add_points(points[i]);
    }
    this->mStream->Write(serverMessage);
  });

  assert(mModelPath != nullptr);
  StrategyPtr opponent = makePlayer(mModelPath);
  StrategyPtr client(new ClientPlayer(mStream));

  StrategyPtr players[4] = {opponent, opponent, opponent, opponent};

  GameOutcome humanOutcome;
  GameOutcome referenceOutcome;

  {
    GameState reference(N);
    referenceOutcome = reference.PlayGame(players, RandomGenerator::ThreadSpecific());
  }
  {
    players[0] = client;
    SendHand(gameState.HandForPlayer(0));
    humanOutcome = gameState.PlayGame(players, RandomGenerator::ThreadSpecific());
  }

  SendHandResult(humanOutcome, referenceOutcome);
}

bool PlayerSession::IsGameOver()
{
  bool gameOver = false;
  auto itMax = std::max_element(mTotals.begin(), mTotals.end());
  if (*itMax >= 100)
  {
    auto itMin = std::min_element(mTotals.begin(), mTotals.end());
    int winner = itMin - mTotals.begin();
    assert(winner >= 0);
    assert(winner < 4);

    // Be sure there isn't a tie
    bool tied = false;
    for (int i = 0; i < 4; i++)
    {
      if (i != winner && mTotals[i] == *itMin)
      {
        tied = true;
        break;
      }
    }
    gameOver = !tied;
  }
  return gameOver;
}

static int toint(float f) { return int(nearbyint(f)); }

void PlayerSession::SendHandResult(const GameOutcome& humanOutcome, const GameOutcome& referenceOutcome)
{
  const float kOffset = 6.5;
  ServerMessage serverMessage;
  playhearts::HandResult* result = serverMessage.mutable_handresult();

  for (int p = 0; p < 4; p++)
  {
    int score = toint(humanOutcome.ZeroMeanStandardScore(p) + kOffset);
    result->add_scores(score);
    mTotals[p] += score;
    result->add_totals(mTotals[p]);

    score = toint(referenceOutcome.ZeroMeanStandardScore(p) + kOffset);
    result->add_referencescores(score);
    mReferenceTotals[p] += score;
    result->add_referencetotals(mReferenceTotals[p]);
  }
  mStream->Write(serverMessage);

  if (IsGameOver())
  {
    SendGameResult();
  }
}

void PlayerSession::SendGameResult()
{
  ServerMessage serverMessage;
  playhearts::GameResult* result = serverMessage.mutable_gameresult();
  auto itMin = std::min_element(mTotals.begin(), mTotals.end());
  int winner = itMin - mTotals.begin();
  assert(winner >= 0);
  assert(winner < 4);
  result->set_winner(winner);
  for (int p = 0; p < 4; p++)
  {
    result->add_totals(mTotals[p]);
    result->add_referencetotals(mReferenceTotals[p]);
  }
  mStream->Write(serverMessage);
  mTotals.fill(0);
  mReferenceTotals.fill(0);
}

void PlayerSession::SendHand(const CardHand& hand)
{
  ServerMessage serverMessage;
  playhearts::Hand* protoHand = serverMessage.mutable_hand();
  playhearts::Cards* protoCards = protoHand->mutable_cards();
  CardHand::iterator it(hand);
  while (!it.done())
  {
    playhearts::Card* protoCard = protoCards->add_card();
    setProtocolCard(protoCard, it.next());
  }
  mStream->Write(serverMessage);
}
