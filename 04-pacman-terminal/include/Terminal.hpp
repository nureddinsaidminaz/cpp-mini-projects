
#pragma once

#include <termios.h>

class Terminal {
public:
    Terminal();
    ~Terminal();

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    int readKey();

private:
    termios original{};
    int originalFlags = -1;
    bool active = false;
};
