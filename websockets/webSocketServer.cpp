#include "CardsWebServer.hpp"

#include <iostream>

int main(int argc, char **argv)
{
    // For now, we use a hard-coded port
    int port = 3001;

    if (argc <= 1)
    {
        std::cerr << "Must provide one argument: the path to the root directory\n";
        exit(1);
    }
    char *root = argv[1];

    cardsws::CardsWebServer app;
    app.launch(root, port);
}
