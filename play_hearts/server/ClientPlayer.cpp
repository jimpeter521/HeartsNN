// play_hearts/server/ClientPlayer.cpp

#include "play_hearts/server/ClientPlayer.h"

#include "lib/Card.h"
#include "lib/KnowableState.h"
#include "lib/random.h"

using playhearts::MyPlay;
using playhearts::YourTurn;

ClientPlayer::~ClientPlayer() {}

ClientPlayer::ClientPlayer(ServerReaderWriter<ServerMessage, ClientMessage>* stream)
    : mStream(stream)
{}

Card fromProtocolCard(const playhearts::Card& protoCard) { return CardFor(Rank(protoCard.rank()), Suit(protoCard.suit())); }

void setProtocolCard(playhearts::Card* protoCard, Card c)
{
  protoCard->set_rank(::playhearts::Rank(RankOf(c)));
  protoCard->set_suit(::playhearts::Suit(SuitOf(c)));
}

void ClientPlayer::sendYourTurn(const KnowableState& knowableState) const
{
  ServerMessage serverMessage;
  YourTurn* yourTurn = serverMessage.mutable_yourturn();

  yourTurn->set_playnumber(knowableState.PlayNumber());

  int trickSuit = knowableState.TrickSuit();
  if (trickSuit != kUnknown)
    yourTurn->set_tricksuit(::playhearts::Suit(trickSuit));

  ::playhearts::Cards* cards = yourTurn->mutable_tricksofar();
  for (unsigned i = 0; i < knowableState.PlayInTrick(); ++i)
  {
    Card onTable = knowableState.GetTrickPlay(i);
    ::playhearts::Card* c = cards->add_card();
    setProtocolCard(c, onTable);
  }

  {
    CardHand choices = knowableState.LegalPlays();
    ::playhearts::Cards* legalPlays = yourTurn->mutable_legalplays();
    CardArray::iterator it(choices);
    while (!it.done())
    {
      ::playhearts::Card* c = legalPlays->add_card();
      setProtocolCard(c, it.next());
    }
  }

  {
    CardArray::iterator it(knowableState.CurrentPlayersHand());
    ::playhearts::Cards* hand = yourTurn->mutable_hand();
    while (!it.done())
    {
      ::playhearts::Card* c = hand->add_card();
      setProtocolCard(c, it.next());
    }
  }

  std::cout << "Sending YourTurn message to client" << std::endl;
  mStream->Write(serverMessage);
}

Card ClientPlayer::receiveMyPlay() const
{
  ClientMessage clientMessage;

  std::cout << "Waiting for MyPlay message from client" << std::endl;
  mStream->Read(&clientMessage);
  if (clientMessage.req_case() != ClientMessage::kMyPlay)
  {
    std::cout << "Exected client message " << ClientMessage::kMyPlay << ", got " << clientMessage.req_case()
              << std::endl;
    assert(false);
  }

  const MyPlay& myPlay = clientMessage.myplay();
  const playhearts::Card card = myPlay.card();

  Card play = CardFor(Rank(card.rank()), Suit(card.suit()));
  std::cout << "Received play" << NameOf(play) << std::endl;

  return play;
}

Card ClientPlayer::choosePlay(const KnowableState& knowableState, const RandomGenerator& rng) const
{
  sendYourTurn(knowableState);
  return receiveMyPlay();
}

Card ClientPlayer::predictOutcomes(
    const KnowableState& state, const RandomGenerator& rng, float playExpectedValue[13]) const
{
  for (int i = 0; i < 13; i++)
    playExpectedValue[i] = 0.0;
  return choosePlay(state, rng);
}
