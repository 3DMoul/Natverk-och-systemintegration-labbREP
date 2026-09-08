#include "reading.hpp"
#include <httplib.h>
#include <iostream>
#include <mutex>
#include <optional>

int main() {
    httplib::Server server;
    std::mutex mutex;
    std::optional<Reading> latest;
    server.set_payload_max_length(4096);
    server.Get("/health", [](const auto&, auto& res) {
        res.set_content("ok", "text/plain");
    });

    server.Post("/api/readings", [&](const auto& req, auto& res) {
        const auto type = req.get_header_value("Content-Type");
        if (type != "application/json" && type != "application/json; charset=utf-8") {
            res.status = 415;
            res.set_content(R"({"error":"expected application/json"})", "application/json");
            return;
        }
        try {
            const Reading reading = parse_reading(req.body);

            // Hold the lock only while updating shared state.
            {
                std::lock_guard<std::mutex> lock(mutex);
                latest = reading;
            }

            res.status = 201;
            res.set_content(serialize_reading(reading), "application/json");
        } catch (const std::exception& error) {
            res.status = 400;
            const nlohmann::json body{{"error", error.what()}};
            res.set_content(body.dump(), "application/json");
        }
    });

    server.Get("/api/readings/latest", [&](const auto&, auto& res) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!latest) {
            res.status = 404;
            res.set_content(R"({"error":"no reading"})", "application/json");
        } else {
            res.set_content(serialize_reading(*latest), "application/json");
        }
    });

    std::cout << "API http://127.0.0.1:8085 (Ctrl+C to stop)" << std::endl;
    if (!server.listen("127.0.0.1", 8085)) {
        std::cerr << "Cannot listen on port 8085\n";
        return 1;
    }
}
