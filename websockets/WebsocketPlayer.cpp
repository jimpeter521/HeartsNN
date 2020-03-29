// websockets/WebsocketPlayer.cpp

#include "websockets/WebsocketPlayer.hpp"

namespace cardsws {

WebsocketPlayer::~WebsocketPlayer()
{
}

WebsocketPlayer::WebsocketPlayer()
{

}

void WebsocketPlayer::sendYourTurn(const KnowableState& knowableState) const
{
    const auto playNumber = knowableState.PlayNumber();

    int trickSuit = knowableState.TrickSuit();
    if (trickSuit != kUnknown)
    {
        // must play in trick suit
    }

    for (unsigned i = 0; i < knowableState.PlayInTrick(); ++i)
    {
        Card onTable = knowableState.GetTrickPlay(i);
    }

    {
        CardHand choices = knowableState.LegalPlays();
        CardArray::iterator it(choices);
    }

    {
        CardArray::iterator it(knowableState.CurrentPlayersHand());
    }

    // send a "your turn" message here and and then we are done
}

// Card WebsocketPlayer::receiveMyPlay() const
// {
//     // We block here until the human has clicked on the card they want to play
//     // and then return the card as the function result
//     CardHand choices = knowableState.LegalPlays();
//     return choices.FirstCard();
// }

Card WebsocketPlayer::choosePlay(const KnowableState& knowableState, const RandomGenerator& rng) const
{
    CardHand choices = knowableState.LegalPlays();
    return choices.FirstCard();
    // sendYourTurn(knowableState);
    // return receiveMyPlay();
}

Card WebsocketPlayer::predictOutcomes(
    const KnowableState& state, const RandomGenerator& rng, float playExpectedValue[13]) const
{
    for (int i = 0; i < 13; i++)
        playExpectedValue[i] = 0.0;
    return choosePlay(state, rng);
}

} // namespace cardsws
