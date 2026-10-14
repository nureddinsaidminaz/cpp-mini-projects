
#include "Terminal.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

Terminal::Terminal() {
    if (!isatty(STDIN_FILENO)) {
        throw std::runtime_error("Interactive terminal required");
    }

    if (tcgetattr(STDIN_FILENO, &original) != 0) {
        throw std::runtime_error("Cannot read terminal settings");
    }

    termios raw = original;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        throw std::runtime_error("Cannot enable raw input");
    }

    originalFlags = fcntl(STDIN_FILENO, F_GETFL, 0);

    if (originalFlags == -1 ||
        fcntl(STDIN_FILENO, F_SETFL,
              originalFlags | O_NONBLOCK) == -1) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
        throw std::runtime_error("Cannot configure input");
    }

    active = true;

    // Alternate screen + clear + hide cursor
    std::cout << "\033[?1049h\033[2J\033[H\033[?25l"
              << std::flush;
}

Terminal::~Terminal() {
    // Restore terminal settings
    if (active) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
        fcntl(STDIN_FILENO, F_SETFL, originalFlags);
    }

    // Restore colors, cursor and normal screen
    std::cout << "\033[0m\033[?25h\033[?1049l"
              << std::flush;
}

int Terminal::readKey() {
    unsigned char key{};

    if (read(STDIN_FILENO, &key, 1) == 1) {
        return key;
    }

    return -1;
}
