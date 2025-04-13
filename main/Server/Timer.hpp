#ifndef TIMER_HPP
#define TIMER_HPP

#include <SDL3/SDL.h>  // Ensure SDL is initialized and available
#include <string>

class Timer {
private:
    int timeLeft;          // Remaining time in seconds, e.g., 180 for 3 minutes
    Uint32 lastUpdateTime; // Last time the timer was updated
public:
    std::string timeStr;   // Formatted string (MM:SS)

    // Constructor initializes the timer with a given number of seconds.
    Timer(int initialTime = 180)
        : timeLeft(initialTime), lastUpdateTime(SDL_GetTicks()), timeStr("3:00") {
    }

    // Updates the timer once per second, returning the current time string.
    std::string UpdateTime() {
        Uint32 currentTime = SDL_GetTicks();
        // Check if at least 1000ms have passed since last update.
        if (currentTime >= lastUpdateTime + 1000) {
            lastUpdateTime += 1000;
            if (timeLeft > 0)
                timeLeft--;
            timeStr = std::to_string(timeLeft / 60) + ":" +
                (timeLeft % 60 < 10 ? "0" : "") +
                std::to_string(timeLeft % 60);
        }
        return timeStr;
    }

    // Returns whether the timer has run out.
    bool isTimeUp() const { return timeLeft <= 0; }

    void GameOverAction();
};

#endif // TIMER_HPP