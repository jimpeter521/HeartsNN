// play_hearts/conversions.h

#pragma once

#include "lib/Card.h"
#include "lib/CardArray.h"
#include "play_hearts.grpc.pb.h"

inline ::Card fromProtocolCard(const playhearts::Card& protoCard)
{
  return CardFor(protoCard.rank(), protoCard.suit());
}

inline void setProtocolCard(playhearts::Card* protoCard, ::Card c)
{
  protoCard->set_rank(::playhearts::Rank(RankOf(c)));
  protoCard->set_suit(::playhearts::Suit(SuitOf(c)));
}

inline CardHand fromProtoCards(const ::playhearts::Cards& cards)
{
  CardHand hand;
  for (int i = 0; i < cards.card_size(); ++i)
    hand.InsertCard(fromProtocolCard(cards.card(i)));
  return hand;
}
