#include <stdexcept>
#include "Game.hpp"
#include "Terminal.hpp"
#include "Pathfinding.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <iostream>
#include <string>
#include <thread>

#include <sys/ioctl.h>
#include <unistd.h>

using namespace std::chrono_literals;

Game::Game() {
    map = {
        "#####################",
        "#.........#.........#",
        "#.###.###.#.###.###.#",
        "#.#.....#...#.....#.#",
        "#.#.###.#####.###.#.#",
        "#...#...........#...#",
        "###.#.###.#.###.#.###",
        "#...#.....#.....#...#",
        "#.#####.#####.#####.#",
        "#...................#",
        "#####################"
    };

    const auto width = map.front().size();

    for (const auto& row : map) {
        if (row.size() != width) {
            throw std::runtime_error("Invalid maze width");
        }
    }

    // Four power pellets
    for (const Position p : std::array<Position, 4>{{
        {1, 3}, {19, 3}, {1, 9}, {19, 9}
    }}) {
        map[p.y][p.x] = 'o';
    }

    for (const auto& row : map) {
        remainingPellets += static_cast<int>(
            std::count_if(
                row.begin(), row.end(),
                [](char c) {
                    return c == '.' || c == 'o';
                }
            )
        );
    }

    // Player starts on a pellet.
    if (map[player.y][player.x] == '.') {
        map[player.y][player.x] = ' ';
        --remainingPellets;
    }

    initializeGhosts();
}

void Game::initializeGhosts() {
    ghosts = {
        {GhostType::Blinky, GhostState::Chasing,
         {9, 9}, {9, 9}, {1, 1}, {}},

        {GhostType::Pinky, GhostState::Chasing,
         {10, 9}, {10, 9}, {1, 1}, {}},

        {GhostType::Inky, GhostState::Chasing,
         {11, 9}, {11, 9}, {1, 1}, {}},

        {GhostType::Clyde, GhostState::Chasing,
         {12, 9}, {12, 9}, {1, 1}, {}}
    };
}

bool Game::isWall(int x, int y) const {
    if (y < 0 || y >= static_cast<int>(map.size())) {
        return true;
    }

    if (x < 0 || x >= static_cast<int>(map[y].size())) {
        return true;
    }

    return map[y][x] == '#';
}

bool Game::powerActive(TimePoint now) const {
    return now < powerUntil;
}

void Game::move(int dx, int dy) {
    const int nx = player.x + dx;
    const int ny = player.y + dy;

    if (isWall(nx, ny)) return;

    player = {nx, ny};

    char& tile = map[ny][nx];

    if (tile == '.') {
        score += 10;
        --remainingPellets;
        tile = ' ';
    } else if (tile == 'o') {
        score += 50;
        --remainingPellets;
        tile = ' ';

        const auto now = Clock::now();
        powerUntil = now + 7s;
        ghostCombo = 0;

        for (auto& ghost : ghosts) {
            if (ghost.state == GhostState::Chasing) {
                ghost.state = GhostState::Frightened;
            }
        }
    }

    checkCollisions(Clock::now());

    if (remainingPellets == 0 && running) {
        won = true;
        running = false;
    }
}

GhostPosition Game::chooseTarget(
    const Ghost& ghost
) const {
    GhostPosition target{player.x, player.y};

    const int dx = player.x - ghost.position.x;
    const int dy = player.y - ghost.position.y;

    switch (ghost.type) {
        case GhostType::Blinky:
            break;

        case GhostType::Pinky:
            // Predict ahead in a simplified way.
            target.x += (dx >= 0 ? 3 : -3);
            break;

        case GhostType::Inky:
            target.y += (dy >= 0 ? 3 : -3);
            break;

        case GhostType::Clyde:
            if (dx * dx + dy * dy < 36) {
                target = {1, 9};
            }
            break;
    }

    if (isWall(target.x, target.y)) {
        return {player.x, player.y};
    }

    return target;
}

void Game::updateStates(TimePoint now) {
    for (auto& ghost : ghosts) {
        if (ghost.state == GhostState::Respawning &&
            now >= ghost.stateUntil) {

            ghost.position = ghost.spawn;
            ghost.state = GhostState::Protected;
            ghost.stateUntil = now + 2s;
        }

        if (ghost.state == GhostState::Protected &&
            now >= ghost.stateUntil) {

            ghost.state = powerActive(now)
                ? GhostState::Frightened
                : GhostState::Chasing;
        }

        if (ghost.state == GhostState::Frightened &&
            !powerActive(now)) {

            ghost.state = GhostState::Chasing;
        }

        if (ghost.state == GhostState::Chasing &&
            powerActive(now)) {

            ghost.state = GhostState::Frightened;
        }
    }
}

void Game::updateGhosts(TimePoint now) {
    updateStates(now);

    const std::array<GhostPosition, 4> directions{{
        {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    }};

    for (auto& ghost : ghosts) {
        if (ghost.state == GhostState::Respawning) {
            continue;
        }

        GhostPosition target = chooseTarget(ghost);

        if (ghost.state == GhostState::Frightened) {
            // Find a random legal neighbouring tile.
            std::vector<GhostPosition> options;

            for (const auto& d : directions) {
                GhostPosition next{
                    ghost.position.x + d.x,
                    ghost.position.y + d.y
                };

                if (!isWall(next.x, next.y)) {
                    options.push_back(next);
                }
            }

            if (!options.empty()) {
                std::uniform_int_distribution<std::size_t> pick(
                    0, options.size() - 1
                );

                ghost.position = options[pick(rng)];
            }
        } else {
            ghost.position = bfsNextStep(
                map, ghost.position, target
            );
        }
    }

    checkCollisions(now);
}

void Game::loseLife(TimePoint now) {
    if (now < playerProtectedUntil) return;

    --lives;

    if (lives <= 0) {
        running = false;
        return;
    }

    player = {1, 1};
    playerProtectedUntil = now + 2s;

    // Cancel the active power mode.
    powerUntil = TimePoint{};
    ghostCombo = 0;

    initializeGhosts();
}

void Game::checkCollisions(TimePoint now) {
    for (auto& ghost : ghosts) {
        if (ghost.state == GhostState::Respawning) {
            continue;
        }

        if (ghost.position.x != player.x ||
            ghost.position.y != player.y) {
            continue;
        }

        if (ghost.state == GhostState::Protected) {
            continue;
        }

        if (ghost.state == GhostState::Frightened &&
            powerActive(now)) {

            const int bonus = 200 << std::min(ghostCombo, 3);

            score += bonus;
            ++ghostCombo;

            ghost.state = GhostState::Respawning;
            ghost.stateUntil = now + 3s;

        } else {
            loseLife(now);
            break;
        }
    }
}

void Game::render() const {
    winsize size{};

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0) {
        return;
    }

    constexpr int minWidth = 52;
    constexpr int minHeight = 18;

    std::string frame = "\033[H";

    if (size.ws_col < minWidth ||
        size.ws_row < minHeight) {

        frame += "\033[2J\033[H";
        frame += "En az 52x18 terminal gerekli.\033[J";
        std::cout << frame << std::flush;
        return;
    }

    const auto now = Clock::now();

    frame += "\033[96m=== PAC-MAN: GHOST INVASION ===\033[0m\033[K\n";

    frame += "Score: " + std::to_string(score);
    frame += " | Lives: " + std::to_string(lives);
    frame += " | Pellets: " + std::to_string(remainingPellets);
    frame += "\033[K\n";

    if (powerActive(now)) {
        const auto seconds = std::chrono::duration_cast<
            std::chrono::seconds
        >(powerUntil - now).count() + 1;

        frame += "\033[96mPOWER: ";
        frame += std::to_string(seconds);
        frame += "s\033[0m";
    } else {
        frame += "POWER: OFF";
    }

    frame += "\033[K\n";

    for (int y = 0; y < static_cast<int>(map.size()); ++y) {
        for (int x = 0; x < static_cast<int>(map[y].size()); ++x) {
            bool occupied = false;

            if (player.x == x && player.y == y) {
                frame += "\033[93m@\033[0m";
                continue;
            }

            for (const auto& ghost : ghosts) {
                if (ghost.state == GhostState::Respawning) {
                    continue;
                }

                if (ghost.position.x == x &&
                    ghost.position.y == y) {

                    const char* color = "\033[91m";

                    if (ghost.state == GhostState::Frightened) {
                        color = "\033[94m";
                    } else if (ghost.state == GhostState::Protected) {
                        color = "\033[97m";
                    } else {
                        switch (ghost.type) {
                            case GhostType::Blinky:
                                color = "\033[91m"; break;
                            case GhostType::Pinky:
                                color = "\033[95m"; break;
                            case GhostType::Inky:
                                color = "\033[96m"; break;
                            case GhostType::Clyde:
                                color = "\033[33m"; break;
                        }
                    }

                    frame += color;
                    frame += ghost.symbol();
                    frame += "\033[0m";

                    occupied = true;
                    break;
                }
            }

            if (occupied) continue;

            switch (map[y][x]) {
                case '#':
                    frame += "\033[94m#\033[0m"; break;
                case '.':
                    frame += "\033[97m.\033[0m"; break;
                case 'o':
                    frame += "\033[95mO\033[0m"; break;
                default:
                    frame += ' '; break;
            }
        }

        frame += "\033[K\n";
    }

    frame += "WASD: Move | Q: Quit\033[K\n";

    if (!running) {
        frame += won ? "YOU WIN!" : "GAME OVER";
    } else {
        frame += "Collect power pellets to eat ghosts!";
    }

    frame += "\033[K\033[J";

    std::cout << frame << std::flush;
}

void Game::run() {
    Terminal terminal;

    using Clock = std::chrono::steady_clock;

    constexpr auto frameDuration = 33ms;
    constexpr auto moveDuration = 115ms;
    constexpr auto ghostDuration = 320ms;

    auto nextFrame = Clock::now();
    auto lastMove = Clock::now();
    auto lastGhostMove = Clock::now();

    int dx = 0;
    int dy = 0;

    while (running) {
        int key;

        while ((key = terminal.readKey()) != -1) {
            switch (std::tolower(
                static_cast<unsigned char>(key)
            )) {
                case 'w': dx = 0; dy = -1; break;
                case 's': dx = 0; dy = 1; break;
                case 'a': dx = -1; dy = 0; break;
                case 'd': dx = 1; dy = 0; break;
                case 'q': running = false; break;
                default: break;
            }
        }

        if (!running) break;

        const auto now = Clock::now();

        updateStates(now);

        if (now - lastMove >= moveDuration) {
            move(dx, dy);
            lastMove = now;
        }

        if (!running) break;

        if (now - lastGhostMove >= ghostDuration) {
            updateGhosts(now);
            lastGhostMove = now;
        }

        render();

        nextFrame += frameDuration;
        std::this_thread::sleep_until(nextFrame);

        if (Clock::now() > nextFrame + frameDuration) {
            nextFrame = Clock::now();
        }
    }

    render();
}
