#pragma once

#include "lib/Strategy.h"

// TODO: Trim down these includes and usings to just the minimum required.
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
using playhearts::ServerMessage;

class ClientPlayer : public Strategy
{
public:
  virtual ~ClientPlayer();
  ClientPlayer(ServerReaderWriter<ServerMessage, ClientMessage>* stream);

  virtual Card choosePlay(const KnowableState& state, const RandomGenerator& rng) const;

  virtual Card predictOutcomes(
      const KnowableState& state, const RandomGenerator& rng, float playExpectedValue[13]) const;

private:
  void sendYourTurn(const KnowableState& knowableState) const;
  Card receiveMyPlay() const;

private:
  ServerReaderWriter<ServerMessage, ClientMessage>* mStream;
};
