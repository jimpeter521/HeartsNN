// play_hearts/AbstractClient.h

#pragma once

#include "play_hearts.grpc.pb.h"
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include <grpc/grpc.h>

class AbstractClient
{
public:
  AbstractClient(std::shared_ptr<grpc::Channel> channel);
  virtual ~AbstractClient(){};

  void PlayOneGame();

  virtual void SendPlayerMessage();

  virtual void ReceiveHelloMessage();

  virtual void SendStartGameMessage();

  virtual void OnHand(const playhearts::Hand& h) = 0;

  virtual void OnCardPlayed(const playhearts::CardPlayed& played) = 0;

  virtual void OnYourTurn(const playhearts::YourTurn& turn) = 0;

  virtual void OnTrickResult(const playhearts::TrickResult& trickResult) = 0;

  virtual void OnHandResult(const playhearts::HandResult& handResult) = 0;

  virtual void OnGameResult(const playhearts::GameResult& gameResult) = 0;

protected:
  playhearts::ClientMessage newClientMessage();

protected:
  std::unique_ptr<playhearts::PlayHearts::Stub> stub_;
  grpc::ClientContext mContext;
  std::shared_ptr<grpc::ClientReaderWriter<playhearts::ClientMessage, playhearts::ServerMessage>> mStream;
  std::string mSessionToken;
};