// websockets/WebsocketPlayer.hpp

#pragma once

#include "lib/KnowableState.h"
#include "lib/Strategy.h"

namespace cardsws {

class WebsocketPlayer : public Strategy
{
public:
  virtual ~WebsocketPlayer();
  WebsocketPlayer();

  virtual Card choosePlay(const KnowableState& state, const RandomGenerator& rng) const;

  virtual Card predictOutcomes(
      const KnowableState& state, const RandomGenerator& rng, float playExpectedValue[13]) const;

private:
  void sendYourTurn(const KnowableState& knowableState) const;
  Card receiveMyPlay() const;

private:
};

} // namespace cardsws
