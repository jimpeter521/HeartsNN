// play_hearts/PlayerSession.h

#pragma once

#include "play_hearts.grpc.pb.h"
#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc/grpc.h>

#include "lib/GameState.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using playhearts::ClientMessage;
using playhearts::MyPlay;
using playhearts::Player;
using playhearts::ServerMessage;
using playhearts::StartGame;

class PlayerSession
{
public:
  PlayerSession(ServerReaderWriter<ServerMessage, ClientMessage>* stream, const char* modelpath);

  Status ManageSession();

  void OnPlayer(const Player& player);
  void OnStartGame(const StartGame& startGame);
  void OnMyPlay(const MyPlay& myplay);

  void SendHand(const CardHand& hand);
  void SendGameResult(const GameOutcome& humanOutcome, const GameOutcome& referenceOutcome);

private:
  ServerReaderWriter<ServerMessage, ClientMessage>* mStream;
  const char* const mModelPath;
  std::string mPlayerName;
  std::string mPlayerEmail;
  std::string mSessionToken;

  GameState* mGameState;
  int mTotals[4] = {0};
  int mReferenceTotals[4] = {0};
};
