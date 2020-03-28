#include "CardsWebServer.hpp"

#include <App.h>

#include "helpers/AsyncFileReader.h"
#include "helpers/AsyncFileStreamer.h"
#include "helpers/Middleware.h"

#include <chrono>
#include <random>

namespace cardsws {

struct CardsWebServer::Impl : public uWS::App
{
    Impl() = default;

    void launch(const std::string& root, int port);
};

void CardsWebServer::launch(const std::string& root, int port)
{
    mImpl = std::make_unique<Impl>();
    mImpl->launch(root, port);
}

CardsWebServer::CardsWebServer()
{}

CardsWebServer::~CardsWebServer()
{}

enum CommandCodes : char
{
    DEALCARD = 0,
    PLAYCARD = 1,
    CLEARTRICK = 2,
    CARDCLICKED = 3,
};

using Socket = uWS::WebSocket<false, true>;

template <size_t size>
using Message = std::array<char, size>;

template <size_t size>
void sendMessage(Socket* ws, const Message<size>& message)
{
    std::string_view msg(message.data(), size);
    ws->send(msg, uWS::BINARY);
}

void clearTrick(Socket* ws)
{
    const Message<1> message{CLEARTRICK};
    sendMessage(ws, message);
};

void dealCard(Socket* ws, char card)
{
    const Message<2> message {DEALCARD, card};
    sendMessage(ws, message);
}

void playCard(Socket* ws, char card, char seat)
{
    const Message<3> message{PLAYCARD, card, seat};
    sendMessage(ws, message);
}

void CardsWebServer::Impl::launch(const std::string& root, int port)
{
    AsyncFileStreamer asyncFileStreamer(root);

    constexpr unsigned kCardsInDeck{52};

    struct PerSocketData
    {
        PerSocketData()
        {
            for (uint8_t i=0; i<kCardsInDeck; ++i) { mDeck[i] = i; }
            unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::shuffle(mDeck.begin(), mDeck.end(), std::default_random_engine(seed));
        }

        static PerSocketData* data(Socket* ws) { return reinterpret_cast<PerSocketData*>(ws->getUserData()); }

        void dealHand(Socket* ws)
        {
            for (int i=0; i<13; i++)
            {
                dealCard(ws, mDeck[i]);
            }
        }

        void dummyTrick(Socket* ws)
        {
            playCard(ws, mDeck[0], SOUTH); // play first card we dealt to human above
            playCard(ws, mDeck[13], WEST);
            playCard(ws, mDeck[14], NORTH);
            playCard(ws, mDeck[15], EAST);
        }

        std::array<uint8_t, kCardsInDeck> mDeck;
    };

    // Serve HTTP
    this->get("/*", [&asyncFileStreamer](auto *res, auto *req) {
        serveFile(res, req);
        asyncFileStreamer.streamFile(res, req->getUrl());
    })

    // Serve WebSocket API
    .ws<PerSocketData>("/*", {
        /* Settings */
        .compression = uWS::DISABLED,
        .maxPayloadLength = 16 * 1024,
        .idleTimeout = 120,
        .maxBackpressure = 1 * 1024 * 1204,
        /* Handlers */
        .open = [](auto *ws, auto *req) {
            /* Open event here, you may access ws->getUserData() which points to a PerSocketData struct */
            clearTrick(ws);
            PerSocketData::data(ws)->dealHand(ws);
            PerSocketData::data(ws)->dummyTrick(ws);
        },
        .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {

            const auto command = message[0];
            assert(command == CARDCLICKED);
            const auto card = Card::make(message[1]);
            std::cout << "Human clicked on card rank:" << card.rank << " suit:" << card.suit << '\n';
        },
        .drain = [](auto *ws) {
            /* Check ws->getBufferedAmount() here */
        },
        .ping = [](auto *ws) {
            /* Not implemented yet */
        },
        .pong = [](auto *ws) {
            /* Not implemented yet */
        },
        .close = [](auto *ws, int code, std::string_view message) {
            /* You may access ws->getUserData() here */
        }
    })

    .listen(port, [port, root](auto *token) {
        if (token) {
            std::cout << "Serving " << root << " over HTTP a " << port << std::endl;
        }
    })

    .run();

    std::cerr << "Failed to listen to port " << port << std::endl;
    exit(1);
}



} // namespace cardsws
