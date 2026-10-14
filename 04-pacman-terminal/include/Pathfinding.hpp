
#pragma once

#include "Ghost.hpp"

#include <array>
#include <queue>
#include <string>
#include <vector>
#include <algorithm>

inline GhostPosition bfsNextStep(
    const std::vector<std::string>& maze,
    GhostPosition start,
    GhostPosition target
) {
    if (maze.empty()) return start;

    const int height = static_cast<int>(maze.size());
    const int width = static_cast<int>(maze[0].size());

    auto valid = [&](GhostPosition p) {
        return p.x >= 0 && p.y >= 0 &&
               p.x < width && p.y < height &&
               maze[p.y][p.x] != '#';
    };

    if (!valid(start) || !valid(target)) return start;
    if (start == target) return start;

    const std::array<GhostPosition, 4> directions{{
        {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    }};

    std::queue<GhostPosition> queue;
    std::vector<std::vector<bool>> visited(
        height, std::vector<bool>(width, false)
    );

    std::vector<std::vector<GhostPosition>> parent(
        height, std::vector<GhostPosition>(width, {-1, -1})
    );

    visited[start.y][start.x] = true;
    queue.push(start);

    while (!queue.empty()) {
        const GhostPosition current = queue.front();
        queue.pop();

        if (current == target) break;

        for (auto direction : directions) {
            GhostPosition next{
                current.x + direction.x,
                current.y + direction.y
            };

            if (!valid(next) || visited[next.y][next.x]) {
                continue;
            }

            visited[next.y][next.x] = true;
            parent[next.y][next.x] = current;
            queue.push(next);
        }
    }

    if (!visited[target.y][target.x]) return start;

    GhostPosition step = target;

    while (!(parent[step.y][step.x] == start)) {
        step = parent[step.y][step.x];

        if (!valid(step)) return start;
    }

    return step;
}
