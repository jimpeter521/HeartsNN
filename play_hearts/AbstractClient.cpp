// play_hearts/AbstractClient.cpp

#include "play_hearts/AbstractClient.h"

#include "play_hearts.grpc.pb.h"
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include <grpc/grpc.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using ::playhearts::CardPlayed;
using ::playhearts::ClientMessage;
using ::playhearts::GameResult;
using ::playhearts::Hand;
using ::playhearts::Player;
using ::playhearts::PlayHearts;
using ::playhearts::ServerMessage;
using ::playhearts::StartGame;
using ::playhearts::TrickResult;
using ::playhearts::YourTurn;

AbstractClient::AbstractClient(std::shared_ptr<Channel> channel)
    : stub_(PlayHearts::NewStub(channel))
    , mContext()
    , mStream(stub_->Connect(&mContext))
{}

void AbstractClient::PlayOneGame()
{
  bool inProgress = true;
  SendStartGameMessage();
  ServerMessage serverMessage;
  while (inProgress && mStream->Read(&serverMessage))
  {
    switch (serverMessage.res_case())
    {
      case ServerMessage::kHello:
      {
        assert(false); // Handled once in PlayerSession()
        break;
      }
      case ServerMessage::kHand:
      {
        OnHand(serverMessage.hand());
        break;
      }
      case ServerMessage::kCardPlayed:
      {
        OnCardPlayed(serverMessage.cardplayed());
        break;
      }
      case ServerMessage::kYourTurn:
      {
        OnYourTurn(serverMessage.yourturn());
        break;
      }
      case ServerMessage::kTrickResult:
      {
        OnTrickResult(serverMessage.trickresult());
        break;
      }
      case ServerMessage::kGameResult:
      {
        OnGameResult(serverMessage.gameresult());
        inProgress = false;
        break;
      }
      case ServerMessage::RES_NOT_SET:
      {
        assert(false);
        break;
      }
    }
  }
}

void AbstractClient::SendPlayerMessage()
{
  ClientMessage clientMessage;
  Player* player = clientMessage.mutable_player();
  player->set_name("Jim");
  player->set_email("jim.lloyd@gmail.com");
  mStream->Write(clientMessage);
  std::cout << "Sent Player " << std::endl;
}

void AbstractClient::ReceiveHelloMessage()
{
  ServerMessage serverMessage;
  mStream->Read(&serverMessage);
  assert(serverMessage.res_case() == ServerMessage::kHello);
  mSessionToken = serverMessage.hello().sessiontoken();
}

ClientMessage AbstractClient::newClientMessage()
{
  ClientMessage clientMessage;
  clientMessage.set_sessiontoken(mSessionToken);
  return clientMessage;
}

void AbstractClient::SendStartGameMessage()
{
  ClientMessage clientMessage = newClientMessage();
  StartGame* startGame = clientMessage.mutable_startgame();
  mStream->Write(clientMessage);
}
