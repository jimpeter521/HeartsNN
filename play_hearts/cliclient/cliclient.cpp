// play_hearts/cliclient/cliclient.cpp

#include "play_hearts/AbstractClient.h"
#include "play_hearts/conversions.h"

#include <assert.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <unistd.h>

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

class PlayHeartsCliClient : public AbstractClient
{
public:
  PlayHeartsCliClient(std::shared_ptr<Channel> channel)
      : AbstractClient(channel)
  {}

  void PrintHand(const CardHand& hand, const char* pre = "Hand: ", const char* post = "\n")
  {
    if (pre)
      printf("%s", pre);
    CardHand::iterator it(hand);
    while (!it.done())
    {
      printf("%s ", NameOf(it.next()));
    }
    if (post)
      printf("%s", post);
  }

  void OnHand(const ::playhearts::Hand& h) { PrintHand(fromProtoCards(h.cards())); }

  void OnCardPlayed(const CardPlayed& played)
  {
    int playNumber = played.playnumber();
    int player = played.player();
    ::Card card = fromProtocolCard(played.card());

    std::cout << "   " << player << ":" << NameOf(card);
  }

  Card getCardInput(const CardHand& hand, const CardHand& legal)
  {
    PrintHand(hand, "\nHand: ");
    Card choice;
    printf("Choose a card:");
    while (true)
    {
      char* line = nullptr;
      size_t linecap = 0;
      ssize_t linelen = getline(&line, &linecap, stdin);
      assert(linelen >= 3); // 23456789TJQKA CDSH

      char rankChar = toupper(line[0]);
      char suitChar = toupper(line[1]);

      Rank rank;
      if (rankChar >= '2' && rankChar <= '9')
        rank = rankChar - '2';
      else if (rankChar == 'T')
        rank = kTen;
      else if (rankChar == 'J')
        rank = kJack;
      else if (rankChar == 'Q')
        rank = kQueen;
      else if (rankChar == 'K')
        rank = kKing;
      else if (rankChar == 'A')
        rank = kAce;
      else
      {
        printf("Not a valid rank char: %c \n", rankChar);
        continue;
      }

      Suit suit;
      if (suitChar == 'C')
        suit = kClubs;
      else if (suitChar == 'D')
        suit = kDiamonds;
      else if (suitChar == 'S')
        suit = kSpades;
      else if (suitChar == 'H')
        suit = kHearts;
      else
      {
        printf("Not a valid suit char: %c \n", suitChar);
        continue;
      }

      choice = CardFor(rank, suit);
      printf("You chose card: %s\n", NameOf(choice));

      if (legal.HasCard(choice))
        break;
      else
      {
        printf("But that is not a legal play!\n");
      }
    }

    return choice;
  }

  void OnYourTurn(const YourTurn& turn)
  {
    CardHand legal = fromProtoCards(turn.legalplays());
    CardHand hand = fromProtoCards(turn.hand());
    Card choice = getCardInput(hand, legal);

    ClientMessage clientMessage = newClientMessage();
    playhearts::MyPlay* myPlay = clientMessage.mutable_myplay();
    playhearts::Card* protoCard = myPlay->mutable_card();
    setProtocolCard(protoCard, choice);

    mStream->Write(clientMessage);
  }

  void OnTrickResult(const TrickResult& trickResult)
  {
    int points[4];
    for (int p = 0; p < 4; ++p)
    {
      points[p] = trickResult.points(p);
    }
    std::cout << "\nTrick winner:" << trickResult.trickwinner() << std::endl;
    std::cout << "Points: " << points[0] << " " << points[1] << " " << points[2] << " " << points[3] << std::endl;
    std::cout << std::endl;
  }

  void OnHandResult(const HandResult& HandResult)
  {
    int scores[4];
    int totals[4];
    int refscores[4];
    int reftotals[4];
    for (int p = 0; p < 4; ++p)
    {
      scores[p] = HandResult.scores(p);
      totals[p] = HandResult.totals(p);
      refscores[p] = HandResult.referencescores(p);
      reftotals[p] = HandResult.referencetotals(p);
    }
    const char* kFormat = "%9s: %3d %3d %3d %3d (Totals: %3d %3d %3d %3d)\n";
    printf(kFormat, "Scores", scores[0], scores[1], scores[2], scores[3], totals[0], totals[1], totals[2], totals[3]);
    printf(kFormat, "Reference", refscores[0], refscores[1], refscores[2], refscores[3], reftotals[0], reftotals[1],
        reftotals[2], reftotals[3]);
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
  PlayHeartsCliClient guide(grpc::CreateChannel("localhost:50057", grpc::InsecureChannelCredentials()));

  guide.PlayerSession();

  return 0;
}
