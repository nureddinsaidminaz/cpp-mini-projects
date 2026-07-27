# C++ Mini Projects

A collection of small C++ projects created while learning core programming, algorithms, memory management, and the C++ standard library.

## Projects

### 01 — Number Analyzer

A command-line program that:

- Reads an integer from the user
- Determines whether it is positive, negative, or zero
- Determines whether it is odd or even

## Build and Run

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic 01-number-analyzer/main.cpp -o app
./app
```bash
cat > .gitignore <<'EOF'
app
*.o
*.out
*.exe
build/
.vscode/
