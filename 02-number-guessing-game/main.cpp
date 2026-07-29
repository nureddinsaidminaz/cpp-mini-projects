
#include <iostream>
#include <random>
#include <sstream>
#include <string>

constexpr int MIN_NUMBER = 1;
constexpr int MAX_NUMBER = 100;
constexpr int MAX_ATTEMPTS = 7;

int read_guess() {
    while (true) {
        std::cout << "Tahminin: ";

        std::string input;
        std::getline(std::cin, input);

        std::istringstream parser(input);

        int guess;
        char extra_character;

        if (!(parser >> guess) || (parser >> extra_character)) {
            std::cout << "Gecersiz giris. Yalnizca tam sayi gir.\n";
            continue;
        }

        if (guess < MIN_NUMBER || guess > MAX_NUMBER) {
            std::cout << MIN_NUMBER << "-" << MAX_NUMBER
                      << " arasinda bir sayi gir.\n";
            continue;
        }

        return guess;
    }
}

bool ask_to_play_again() {
    while (true) {
        std::cout << "Tekrar oynamak ister misin? (e/h): ";

        std::string answer;
        std::getline(std::cin, answer);

        if (answer == "e" || answer == "E") {
            return true;
        }

        if (answer == "h" || answer == "H") {
            return false;
        }

        std::cout << "Lutfen e veya h gir.\n";
    }
}

void play_game(std::mt19937& generator) {
    std::uniform_int_distribution<int> distribution(
        MIN_NUMBER,
        MAX_NUMBER
    );

    const int secret_number = distribution(generator);

    std::cout << "\n" << MIN_NUMBER << "-" << MAX_NUMBER
              << " arasindaki sayiyi tahmin et.\n";
    std::cout << MAX_ATTEMPTS << " hakkin var.\n\n";

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
        std::cout << "[" << attempt << "/" << MAX_ATTEMPTS << "] ";

        const int guess = read_guess();

        if (guess == secret_number) {
            std::cout << "Dogru tahmin! Sayiyi "
                      << attempt << " denemede buldun.\n";
            return;
        }

        if (guess < secret_number) {
            std::cout << "Daha buyuk bir sayi dene.\n";
        } else {
            std::cout << "Daha kucuk bir sayi dene.\n";
        }

        std::cout << "Kalan hak: "
                  << MAX_ATTEMPTS - attempt << "\n\n";
    }

    std::cout << "Kaybettin. Gizli sayi: "
              << secret_number << "\n";
}

int main() {
    std::random_device random_device;
    std::mt19937 generator(random_device());

    std::cout << "=== Number Guessing Game ===\n";

    do {
        play_game(generator);
    } while (ask_to_play_again());

    std::cout << "Oyun kapatildi.\n";

    return 0;
}