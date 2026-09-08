#include "reading.hpp"
#include <httplib.h>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::runtime_error("Usage: bridge RECEIVED.json");
        }

        std::ifstream in(argv[1], std::ios::binary);
        if (!in) {
            throw std::runtime_error("cannot open input file");
        }

        std::string text((std::istreambuf_iterator<char>(in)), {});
        if (text.size() > 4096) {
            throw std::runtime_error("payload exceeds 4096 bytes");
        }

        const auto reading = parse_reading(text);
        const auto body = serialize_reading(reading);

        httplib::Client client("127.0.0.1", 8085);
        client.set_connection_timeout(2, 0);
        client.set_read_timeout(2, 0);
        client.set_write_timeout(2, 0);

        const auto result = client.Post("/api/readings", body, "application/json");
        if (!result) {
            throw std::runtime_error(
                "API transport failure: " + httplib::to_string(result.error())
            );
        }

        std::cout << "HTTP " << result->status << ' ' << result->body << '\n';
        if (result->status != 201) {
            return 2;
        }
    } catch (const std::exception& error) {
        std::cerr << "Bridge: " << error.what() << '\n';
        return 1;
    }
}
