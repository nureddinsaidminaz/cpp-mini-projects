#include <iostream>

int main() {
    int sayi;
    std::cout << "Bir tam sayi giriniz: ";
    std::cin >> sayi;
    if (sayi > 0) {
        std::cout << "Girdiginiz sayi pozitiftir." << std::endl;
    } else if (sayi < 0) {
        std::cout << "Girdiginiz sayi negatiftir." << std::endl;
    } else {
        std::cout << "Girdiginiz sayi sifirdir." << std::endl;
    }
    if (sayi % 2 == 0) {
        std::cout << "Girdiginiz sayi cifttir." << std::endl;
    } else {
        std::cout << "Girdiginiz sayi tektir." << std::endl;
    }
    return 0;
}