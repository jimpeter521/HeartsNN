#include "lib/Strategy.h"
#include "lib/Annotator.h"
#include "lib/DnnModelIntuition.h"
#include "lib/MonteCarlo.h"
#include "lib/RandomStrategy.h"

Strategy::~Strategy() {}

Strategy::Strategy(const AnnotatorPtr& annotator)
    : mAnnotator(annotator)
{}

StrategyPtr loadIntuition(const std::string& intuitionNameOrPath)
{
    if (intuitionNameOrPath == "random")
    {
        StrategyPtr intuition(new RandomStrategy());
        return intuition;
    }
    else
    {
        StrategyPtr intuition(new DnnModelIntuition(intuitionNameOrPath));
        return intuition;
    }
}

std::vector<std::string> split(const std::string& s, char delimiter = ' ')
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

StrategyPtr makePlayer(const std::string& intuitionName, int rollouts)
{
    StrategyPtr intuition = loadIntuition(intuitionName);
    if (rollouts == 0)
    {
        return intuition;
    }
    else
    {
        AnnotatorPtr kNoAnnotator(0);
        const bool kParallel = true;
        return StrategyPtr(new MonteCarlo(intuition, rollouts, kParallel, kNoAnnotator));
    }
}

StrategyPtr makePlayer(const std::string& arg)
{
    const int kDefaultRollouts = 40;

    std::string intuitionName;
    int rollouts;

    const char kSep = '#';
    if (arg[arg.size() - 1] == kSep)
    {
        intuitionName = arg.substr(0, arg.size() - 1);
        rollouts = kDefaultRollouts;
    }
    else
    {
        std::vector<std::string> parts = split(arg, '#');
        assert(parts.size() > 0);
        assert(parts.size() <= 2);

        intuitionName = parts[0];

        if (parts.size() == 1)
        {
            rollouts = 0;
        }
        else
        {
            rollouts = std::stoi(parts[1]);
        }
    }
    return makePlayer(intuitionName, rollouts);
}
