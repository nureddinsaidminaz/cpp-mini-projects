
#pragma once

#include <chrono>
#include <string>

enum class GhostType {
    Blinky,
    Pinky,
    Inky,
    Clyde
};

enum class GhostState {
    Chasing,
    Frightened,
    Respawning,
    Protected
};

struct GhostPosition {
    int x = 0;
    int y = 0;

    bool operator==(const GhostPosition&) const = default;
};

struct Ghost {
    GhostType type;
    GhostState state = GhostState::Chasing;

    GhostPosition position;
    GhostPosition spawn;
    GhostPosition target;

    std::chrono::steady_clock::time_point stateUntil{};

    char symbol() const {
        switch (type) {
            case GhostType::Blinky: return 'B';
            case GhostType::Pinky:  return 'P';
            case GhostType::Inky:  return 'I';
            case GhostType::Clyde: return 'C';
        }
        return '?';
    }
};
