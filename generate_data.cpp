#include <iostream>
#include <string>
#include <charconv>
#include <random>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 2)
        return 1;
    size_t count;
    auto fr = std::from_chars(argv[1], argv[1] + strlen(argv[1]), count);
    if (fr.ec != std::errc())
        return 2;

    std::default_random_engine e;
    std::uniform_int_distribution char_dist(1, 127);
    std::uniform_int_distribution length_dist(1, 1024);
    std::uniform_int_distribution spec_dist(0, 10);
    for (size_t i = 0; i != count; ++i) {
        size_t ul = length_dist(e);
        for (size_t j = 0; j != ul; ++j) {
            switch (spec_dist(e)) {
            case 0:
                std::cout << "\uffff";
                break;
            case 1:
                std::cout << "\ufffe";
                break;
            default:
                std::cout << static_cast<char>(char_dist(e));
            }
        }
        std::cout << '\n';
    }
    return 0;
}

