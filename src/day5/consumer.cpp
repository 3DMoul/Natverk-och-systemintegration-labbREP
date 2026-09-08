#include "reading.hpp"
#include <httplib.h>
#include <iostream>

int main() {
    try {
        httplib::Client client("127.0.0.1", 8085);
        client.set_connection_timeout(2, 0);
        client.set_read_timeout(2, 0);

        const auto result = client.Get("/api/readings/latest");
        if (!result) {
            throw std::runtime_error(
                "API transport failure: " + httplib::to_string(result.error())
            );
        }

        if (result->status != 200) {
            throw std::runtime_error(
                "HTTP " + std::to_string(result->status) + " " + result->body
            );
        }

        const auto reading = parse_reading(result->body);
        std::cout << reading.sensor_id << ": "
                  << reading.value << ' ' << reading.unit << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Consumer: " << error.what() << '\n';
        return 1;
    }
}
