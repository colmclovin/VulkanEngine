
#include <iostream>
#include <stdexcept>
#include "Game/Game.h"

int main()
{

    try {
        Game game;

        game.Run();

        std::cout << "Exiting normally" << std::endl;
        return EXIT_SUCCESS;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

}