#include "lib/PossibilityAnalyzer.h"
#include "lib/NoVoidsAnalyzer.h"
#include "lib/OneOpponentGetsSuit.h"
#include "lib/TwoOpponentsGetSuit.h"

#include <iostream>
#include <fstream>
#include <sstream>

PossibilityAnalyzer::PossibilityAnalyzer()
{
}

PossibilityAnalyzer::~PossibilityAnalyzer()
{
}

PossibilityAnalyzer* BuildAnalyzer(unsigned player, const VoidBits& voidBits, PriorityList& priorityList, const CardDeck& remaining, const CardHands& hands)
{
  voidBits.VerifyVoids(hands);

  const VoidBits otherVoids = voidBits.ForOthers(player);
  bool done = false;
  SuitVoids suitVoids;
  while (true) {
    done = priorityList.empty();
    if (done)
      break;

    suitVoids = priorityList.back();
    priorityList.pop_back();
    if (suitVoids.numVoids != 3)
      break;
  }

  if (done || suitVoids.numVoids == 0) {
    if (remaining.Size()) {
      return new NoVoidsAnalyzer(remaining, hands, otherVoids);
    } else {
      return new NoneRemaining();
    }
  }

  Suit suit = suitVoids.suit;
  CardDeck remainingOfSuit, otherRemaining;
  remaining.PartitionRemaining(suit, remainingOfSuit, otherRemaining);

  PossibilityAnalyzer* parent = 0;
  switch(suitVoids.numVoids)
  {
    case 2:
    {
      parent = new OneOpponentGetsSuit(player, otherVoids, suit, remainingOfSuit, otherRemaining, hands);
      break;
    }
    case 1:
    {
      parent = new TwoOpponentsGetSuit(player, otherVoids, suit, remainingOfSuit, otherRemaining, hands);
      break;
    }
    default:
      assert(false);
  }

  parent->AddStage(otherRemaining, priorityList);
  return parent;
}

void PossibilityAnalyzer::RenderDotToFile(const std::string& path, const CardDeck& unknownCardsRemaining) const
{
  std::ofstream file;
  file.open(path);

  file << "digraph {" << std::endl;
  file << "cards [label=\"" << unknownCardsRemaining.AsString() << "\"];" << std::endl;
  this->RenderDot(file);
  file << "}" << std::endl;

  file.close();
}

std::string PossibilityAnalyzer::RenderDotToString() const
{
  std::stringstream ss;
  ss << "digraph {" << std::endl;
  this->RenderDot(ss);
  ss << "}" << std::endl;
  return ss.str();
}

Impossible::Impossible()
{
}

Impossible::~Impossible()
{
}

uint128_t Impossible::Possibilities() const
{
  return 0;
}

void Impossible::ActualizePossibility(uint128_t possibility_index, CardHands& hands) const
{
  assert(false);
}

void Impossible::AddStage(const CardDeck& other_remaining, PriorityList& list)
{
}

void Impossible::RenderDot(std::ostream& stream) const
{
  stream << "id_" << uint64_t(this) << " [label=\"Impossible\"];" << std::endl;
}

void Impossible::ExpectedDistribution(Distribution& distribution, CardHands& hands)
{
  assert(false);
}

NoneRemaining::NoneRemaining()
{
}

NoneRemaining::~NoneRemaining()
{
}

uint128_t NoneRemaining::Possibilities() const
{
  return 1;
}

void NoneRemaining::ActualizePossibility(uint128_t possibility_index, CardHands& hands) const
{
  assert(possibility_index == 0);
  for (int i=0; i<4; ++i)
    assert(hands[i].AvailableCapacity() == 0);
}

void NoneRemaining::AddStage(const CardDeck& other_remaining, PriorityList& list)
{
  assert(other_remaining.Size() == 0);
  assert(list.size() == 0);
}

void NoneRemaining::RenderDot(std::ostream& stream) const
{
  stream << "id_" << uint64_t(this) << " [label=\"NoneRemaining\"];" << std::endl;
}
