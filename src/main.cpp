// src/main.cpp
// Simplified main entry point using refactored Game class
// Replaces the original 1000+ line main.cpp

#include "core/Game.h"
#include "utils/Logger.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        
        // Parse optional --seed argument
        unsigned int cmd_seed = 0;

        for (int i = 1; i < argc - 1; i++) {
            if (std::string(argv[i]) == "--seed") {
                cmd_seed = static_cast<unsigned int>(std::stoul(argv[i + 1]));
                std::cout << "Using seed from command line: " << cmd_seed << std::endl;
            }
        }
        

        // Loop so the player can restart after death/victory without
        // relaunching the executable. Each iteration is a fresh Game, torn
        // down and rebuilt through the same initialize()/run() path as a
        // normal launch, so restart carries no more risk than a cold start.
        bool play_again = true;
        while (play_again) {
            Game game;

            if (!game.initialize(cmd_seed)) {
                LOG_ERROR("Failed to initialize game");
                return 1;
            }

            game.run();
            play_again = game.should_restart();
        }

        // Clean exit
        return 0;

    }
    catch (const std::exception& e) {
        // Handle any uncaught exceptions
        LOG_ERROR("Fatal error: " + std::string(e.what()));
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}