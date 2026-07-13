// Main entry point for the Factorio-like 3D game
// This is a minimal main.cpp that uses the new architecture

#include "GameApp.h"
#include <iostream>

int main() {
	std::cout << "========================================" << std::endl;
	std::cout << "  Factorio-Like 3D Game Engine" << std::endl;
	std::cout << "========================================" << std::endl;

	try {
		GameApp game;
		game.Run();

		std::cout << "Exiting normally" << std::endl;
		return 0;

	} catch (const std::exception& e) {
		std::cerr << "FATAL ERROR: " << e.what() << std::endl;
		return -1;
	}
}

/*
 * CONTROLS:
 * 
 * WASD    - Move camera target (pan)
 * Q/E     - Rotate camera around target
 * R/F     - Zoom in/out
 * ESC     - Quit
 * 
 * TODO: Add mouse controls
 * - Middle mouse drag for panning
 * - Scroll wheel for zoom
 * - Right click for rotation
 * - Left click for building placement
 */
