#pragma once

#include "lib/Annotator.h"
#include "lib/Card.h"
#include "lib/CardArray.h"

#include <memory>

class KnowableState;
class Strategy;

typedef std::shared_ptr<Strategy> StrategyPtr;

class Strategy
{
public:
    virtual ~Strategy();
    Strategy(const AnnotatorPtr& annotator = 0);

    virtual Card choosePlay(const KnowableState& state, const RandomGenerator& rng) const = 0;

    AnnotatorPtr getAnnotator() const { return mAnnotator; }

private:
    const AnnotatorPtr mAnnotator;
};

StrategyPtr makePlayer(const std::string& intuitionName, int rollouts);
StrategyPtr makePlayer(const std::string& arg);
