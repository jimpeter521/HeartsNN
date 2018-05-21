// play_hearts/testclient/play_hearts_client.cpp

#include "play_hearts/AbstractClient.h"

#include <assert.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "play_hearts/conversions.h"

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
using ::playhearts::Hand;
using ::playhearts::HandResult;
using ::playhearts::Player;
using ::playhearts::PlayHearts;
using ::playhearts::ServerMessage;
using ::playhearts::StartGame;
using ::playhearts::TrickResult;
using ::playhearts::YourTurn;

class PlayHeartsClient : public AbstractClient
{
public:
  PlayHeartsClient(std::shared_ptr<Channel> channel)
      : AbstractClient(channel)
  {}

  void OnHand(const ::playhearts::Hand& hand) { const ::playhearts::Cards& cards = hand.cards(); }

  void OnCardPlayed(const CardPlayed& played)
  {
    int playNumber = played.playnumber();
    int player = played.player();
    ::Card card = fromProtocolCard(played.card());

    std::cout << "On play " << playNumber << ", Player " << player << " played card " << NameOf(card) << std::endl;
  }

  void OnYourTurn(const YourTurn& turn)
  {
    const playhearts::Cards& cards = turn.legalplays();
    Card choice = fromProtocolCard(cards.card(0));

    ClientMessage clientMessage = newClientMessage();
    playhearts::MyPlay* myPlay = clientMessage.mutable_myplay();
    playhearts::Card* protoCard = myPlay->mutable_card();
    setProtocolCard(protoCard, choice);

    std::cout << "Client sending choice " << NameOf(choice) << std::endl;
    mStream->Write(clientMessage);
  }

  void OnTrickResult(const TrickResult& trickResult)
  {
    int points[4];
    for (int p = 0; p < 4; ++p)
    {
      points[p] = trickResult.points(p);
    }
    std::cout << "Trick winner:" << trickResult.trickwinner();
    std::cout << "points: " << points[0] << " " << points[1] << " " << points[2] << " " << points[3] << std::endl;
  }

  void OnHandResult(const HandResult& HandResult)
  {
    float scores[4];
    float totals[4];
    for (int p = 0; p < 4; ++p)
    {
      scores[p] = HandResult.scores(p);
      totals[p] = HandResult.totals(p);
    }
    std::cout << "Scores: " << scores[0] << " " << scores[1] << " " << scores[2] << " " << scores[3] << std::endl;
    std::cout << "Totals: " << totals[0] << " " << totals[1] << " " << totals[2] << " " << totals[3] << std::endl;
  }

  void PlayerSession()
  {
    std::cout << "-------------- PlayerSession --------------" << std::endl;
    SendPlayerMessage();
    ReceiveHelloMessage();

    for (int i = 0; i < 100; ++i)
      PlayOneGame();

    Status status = mStream->Finish();
  }
};

int main(int argc, char** argv)
{
  PlayHeartsClient guide(grpc::CreateChannel("localhost:50057", grpc::InsecureChannelCredentials()));

  guide.PlayerSession();

  return 0;
}
