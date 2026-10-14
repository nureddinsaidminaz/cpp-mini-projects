
#pragma once

#include "Ghost.hpp"

#include <chrono>
#include <random>
#include <string>
#include <vector>

struct Position {
    int x = 1;
    int y = 1;
};

class Game {
public:
    Game();
    void run();

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    std::vector<std::string> map;
    std::vector<Ghost> ghosts;

    Position player{1, 1};

    int score = 0;
    int lives = 3;
    int remainingPellets = 0;
    int ghostCombo = 0;

    bool running = true;
    bool won = false;

    TimePoint powerUntil{};
    TimePoint playerProtectedUntil{};

    std::mt19937 rng{std::random_device{}()};

    void render() const;
    void move(int dx, int dy);

    bool isWall(int x, int y) const;
    bool powerActive(TimePoint now) const;

    void initializeGhosts();
    void updateGhosts(TimePoint now);
    void updateStates(TimePoint now);
    void checkCollisions(TimePoint now);

    GhostPosition chooseTarget(
        const Ghost& ghost
    ) const;

    void loseLife(TimePoint now);
};
