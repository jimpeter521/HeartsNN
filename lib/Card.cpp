#include "lib/Card.h"
#include <string>
#include <assert.h>

static const char* kSuits[] = {"♣️", "♦️", "♠️", "♥️"};
static const char* kRanks[] = {" 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", "10", " J", " Q", " K", " A"};

static std::string kNames[kCardsPerDeck];
static unsigned kCardPoints[kCardsPerDeck];

static void InitializeCardNames() {
  for (int i = 0; i<kCardsPerDeck; i++) {
    int rank = i % kCardsPerSuit;
    int suit = i / kCardsPerSuit;
    std::string s(kRanks[rank]);
    s += kSuits[suit];
    kNames[i] = s;
  }
}

static void InitializecardPoints() {
  for (int i = 0; i<kCardsPerDeck; i++) {
    int rank = i % kCardsPerSuit;
    int suit = i / kCardsPerSuit;
    if (suit == kHearts)
      kCardPoints[i] = 1u;
    else if (suit == kSpades && rank == kQueen)
      kCardPoints[i] = 13u;
    else
      kCardPoints[i] = 0u;
  }
}

class Initializer
{
public:
  Initializer()
  {
    InitializeCardNames();
    InitializecardPoints();
  }
};

static Initializer gInitializer;

const char* NameOfSuit(Suit suit)
{
  return suit == kUnknown ? "?" : kSuits[suit];
}

const char* NameOf(Card card)
{
  assert(card < kCardsPerDeck);
  return kNames[card].c_str();
}

unsigned PointsFor(Card card)
{
  assert(card < kCardsPerDeck);
  return kCardPoints[card];
}
