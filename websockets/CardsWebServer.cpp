#include "websockets/CardsWebServer.hpp"
#include "websockets/WebsocketPlayer.hpp"
#include "lib/VisibleState.hpp"
#include "lib/GameState.h"
#include "lib/random.h"

#include <App.h>

#include "helpers/AsyncFileReader.h"
#include "helpers/AsyncFileStreamer.h"
#include "helpers/Middleware.h"

#include <chrono>
#include <random>

namespace cardsws {

using namespace visible;

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

    struct PerSocketData
    {
        PerSocketData()
        {
            const std::string opponentName{"random"};
            StrategyPtr opponent = makePlayer(opponentName);
            // StrategyPtr human(new WebsocketPlayer());
            players[0] = opponent;
            players[1] = players[2] = players[3] = opponent;

            // gameState.SetPlayCardHook([this](int play, int player, Card card) {
            //     // This hook will get called for every play

            //     // ServerMessage serverMessage;
            //     // playhearts::CardPlayed* cardPlayed = serverMessage.mutable_cardplayed();
            //     // cardPlayed->set_playnumber(gameState.PlayNumber());
            //     // cardPlayed->set_player(gameState.CurrentPlayer());
            //     // setProtocolCard(cardPlayed->mutable_card(), card);
            //     // this->mStream->Write(serverMessage);
            // });

            // gameState.SetTrickResultHook([this](int trickWinner, const std::array<unsigned, 4>& points) {
            //     // This hook will get called at the end of every trick

            //     // ServerMessage serverMessage;
            //     // playhearts::TrickResult* trickResult = serverMessage.mutable_trickresult();
            //     // trickResult->set_trickwinner(trickWinner);
            //     // for (int i = 0; i < 4; i++)
            //     // {
            //     //     trickResult->add_points(points[i]);
            //     // }
            //     // this->mStream->Write(serverMessage);
            // });

            #if 0
            assert(gameState.PlayNumber() == 0);
            if (gameState.CurrentPlayer() == VisibleState::SOUTH)
            {
                gameState.AdvanceOnePlay(players, rng);
            }
            while (gameState.CurrentPlayer() != VisibleState::SOUTH)
            {
                assert(gameState.PlayNumber() < 8);
                gameState.AdvanceOnePlay(players, rng);
            }
            #endif
        }

        static PerSocketData* data(Socket* ws) { return reinterpret_cast<PerSocketData*>(ws->getUserData()); }

        void displayHand(Socket* ws)
        {
            auto visible = gameState.asVisibleState(VisibleState::SOUTH);
            CardHand::iterator it(visible.hand());
            while (!it.done()) {
                dealCard(ws, it.next());
            }
        }

        void displayTrick(Socket* ws)
        {
            auto visible = gameState.asVisibleState(VisibleState::SOUTH);
            auto plays = visible.plays();
            for (auto p=0; p<4; ++p)
            {
                if (plays[p] != kNoCard)
                {
                    playCard(ws, plays[p], p); // play first card we dealt to human above
                }
            }
        }

        RandomGenerator rng;
        uint128_t N{Deal::RandomDealIndex()};
        Deal deal{N};
        GameState gameState{deal};
        StrategyPtr players[4];
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
            // PerSocketData::data(ws)->displayHand(ws);
            // PerSocketData::data(ws)->displayTrick(ws);
        },
        .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {

            const auto command = message[0];
            assert(command == CARDCLICKED);
            const auto card = message[1];
            std::cout << "Human clicked on card rank:" << NameOf(card) << '\n';
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
