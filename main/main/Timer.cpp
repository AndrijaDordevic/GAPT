#include "TextRender.hpp"
#include "Timer.hpp"
#include <string>
#include <iostream>

int timeLeft = 180;  // 3-minute countdown
Uint32 lastUpdateTime = 0;

std::string timeStr = "3";


std::string UpdateTime() {

	Uint32 currentTime = SDL_GetTicks();

	if (currentTime > lastUpdateTime + 1000) {

		lastUpdateTime = currentTime;

		if (timeLeft > 0) timeLeft--;

		// Convert seconds to MM:SS format
		timeStr = std::to_string(timeLeft / 60) + ":" +
			(timeLeft % 60 < 10 ? "0" : "") + std::to_string(timeLeft % 60);
	}
	return timeStr;
}

void GameOverCheck() {

	if (timeLeft == 0) {
		// Example action: print message and stop the game
		std::cout << "Time's up! The game is over!";
		//running = false;  // End the game, you can change this to whatever action you prefer

	}
}
