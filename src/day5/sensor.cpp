#include "reading.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::runtime_error("Usage: sensor OUTPUT.json");
        }

        const Reading reading{"temp-01", 21.7, "C"};
        const auto text = serialize_reading(reading);

        std::ofstream out(argv[1], std::ios::binary);
        out << text;
        if (!out) {
            throw std::runtime_error("cannot write output file");
        }

        std::cout << text << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
