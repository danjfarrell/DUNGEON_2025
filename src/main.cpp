// src/main.cpp
// Simplified main entry point using refactored Game class
// Replaces the original 1000+ line main.cpp

#include "core/Game.h"
#include "utils/Logger.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        // Create game instance
        Game game;

        // Initialize all systems
        if (!game.initialize()) {
            LOG_ERROR("Failed to initialize game");
            return 1;
        }

        // Run main game loop
        game.run();

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