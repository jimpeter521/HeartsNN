// play_hearts/PlayerSession.cpp

#include "play_hearts/server/PlayerSession.h"

#include "lib/random.h"
#include "play_hearts/conversions.h"
#include "play_hearts/server/ClientPlayer.h"

using playhearts::Hello;

PlayerSession::PlayerSession(ServerReaderWriter<ServerMessage, ClientMessage>* stream, const char* modelpath)
    : mStream(stream)
    , mModelPath(modelpath)
    , mGameState(nullptr)
{
  assert(mModelPath != nullptr);
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

  assert(mGameState == nullptr);

  uint128_t N = Deal::RandomDealIndex();
  mGameState = new GameState(N);

  mGameState->SetPlayCardHook([this](int play, int player, Card card) {
    ServerMessage serverMessage;
    playhearts::CardPlayed* cardPlayed = serverMessage.mutable_cardplayed();
    cardPlayed->set_playnumber(this->mGameState->PlayNumber());
    cardPlayed->set_player(this->mGameState->CurrentPlayer());
    setProtocolCard(cardPlayed->mutable_card(), card);
    this->mStream->Write(serverMessage);
  });

  mGameState->SetTrickResultHook([this](int trickWinner, const std::array<unsigned, 4>& points) {
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
    SendHand(mGameState->HandForPlayer(0));
    humanOutcome = mGameState->PlayGame(players, RandomGenerator::ThreadSpecific());
  }

  SendGameResult(humanOutcome, referenceOutcome);

  delete mGameState;
  mGameState = nullptr;
}

void PlayerSession::SendGameResult(const GameOutcome& humanOutcome, const GameOutcome& referenceOutcome)
{
  ServerMessage serverMessage;
  playhearts::GameResult* result = serverMessage.mutable_gameresult();
  for (int p = 0; p < 4; p++)
  {
    float score = humanOutcome.ZeroMeanStandardScore(p);
    result->add_scores(score);
    mTotals[p] += score;
    result->add_totals(mTotals[p]);

    score = referenceOutcome.ZeroMeanStandardScore(p);
    result->add_referencescores(score);
    mReferenceTotals[p] += score;
    result->add_referencetotals(mReferenceTotals[p]);
  }
  mStream->Write(serverMessage);
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
