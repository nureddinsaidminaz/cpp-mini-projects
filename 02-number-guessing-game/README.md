
# C++ Mini Projects

A growing collection of small C++ projects built to learn core language concepts, defensive programming, the Standard Library, compilation, and Git workflows.

## Projects

| No. | Project | Main Concepts |
|---|---|---|
| 01 | Number Analyzer | Input/output, conditionals, modulo |
| 02 | Number Guessing Game | Functions, loops, random numbers, input validation |

## 01 — Number Analyzer

A command-line program that:

- Reads an integer from the user
- Determines whether it is positive, negative, or zero
- Determines whether it is odd or even

### Concepts

- `int`
- `std::cin`
- `std::cout`
- `if / else`
- Modulo operator `%`

## 02 — Number Guessing Game

A defensive command-line guessing game that:

- Generates a random integer between 1 and 100
- Gives higher or lower hints
- Limits the player to seven attempts
- Rejects empty, textual, mixed, and out-of-range input
- Counts attempts
- Reveals the secret number after a loss
- Supports replaying without restarting the program

### Concepts

- Functions
- `for`, `while`, and `do-while`
- `constexpr`
- References
- `<random>`
- `<string>`
- `<sstream>`
- Input validation
- Early return

## Build and Run

Enter a project directory:

```bash
cd 02-number-guessing-game
```

Compile with C++20 and strict warnings:

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic main.cpp -o app
```

Run:

```bash
./app
```

## Manual Verification

The Number Guessing Game was checked with:

- Empty input
- Text input such as `abc`
- Mixed input such as `12abc`
- Values below 1
- Values above 100
- Correct guesses
- Seven failed attempts
- Replay and exit commands

## Repository Structure

```text
cpp-mini-projects/
├── 01-number-analyzer/
│   └── main.cpp
├── 02-number-guessing-game/
│   └── main.cpp
├── .gitignore
└── README.md
```

## Development Environment

- C++20
- GCC
- Git
- Visual Studio Code
- Linux

## Goal

Python remains the primary language for larger applications and AI projects. C++ mini projects are used to develop stronger foundations in algorithms, memory, performance, and systems programming.
