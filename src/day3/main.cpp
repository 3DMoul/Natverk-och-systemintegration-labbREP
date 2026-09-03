#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <httplib.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool valid_reading(const json& value, std::string& error) {
    if (!value.contains("sensorId") || !value["sensorId"].is_string()) { error = "sensorId must be a string"; return false; }
    if (!value.contains("value") || !value["value"].is_number()) { error = "value must be a number"; return false; }
    if (!value.contains("unit") || !value["unit"].is_string()) { error = "unit must be a string"; return false; }
    return true;
}

void send_json(httplib::Response& response, int status, const json& body) {
    response.status = status;
    response.set_content(body.dump(2), "application/json");
}

int main() {
    httplib::Server server;
    std::mutex mutex;
    std::optional<json> latest;
    server.Get("/health", [](const httplib::Request&, httplib::Response& response) { send_json(response, 200, {{"status", "ok"}}); });
    server.Get("/api/readings/latest", [&](const httplib::Request&, httplib::Response& response) {
        std::scoped_lock lock(mutex);
        if (!latest) { send_json(response, 404, {{"error", "no reading available"}}); return; }
        send_json(response, 200, *latest);
    });
    server.Post("/api/readings", [&](const httplib::Request& request, httplib::Response& response) {
        if (!request.has_header("Content-Type") || request.get_header_value("Content-Type").find("application/json") != 0) {
            send_json(response, 415, {{"error", "Content-Type must be application/json"}}); return;
        }
        try {
            json reading = json::parse(request.body);
            std::string error;
            if (!valid_reading(reading, error)) { send_json(response, 400, {{"error", error}}); return; }
            { std::scoped_lock lock(mutex); latest = reading; }
            send_json(response, 201, {{"status", "stored"}, {"reading", reading}});
        } catch (const json::parse_error&) { send_json(response, 400, {{"error", "invalid JSON"}}); }
    });
    server.set_error_handler([](const httplib::Request&, httplib::Response& response) {
        if (response.status == 404) send_json(response, 404, {{"error", "endpoint not found"}});
    });
    std::cout << "API lyssnar pa http://127.0.0.1:8080\n";
    return server.listen("127.0.0.1", 8080) ? 0 : 1;
}
