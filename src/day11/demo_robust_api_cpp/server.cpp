#include "net.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <thread>

std::atomic<bool> running{true};
std::map<std::string, int> attempts;

std::string receive_request(socket_t client)
{
    std::string request;
    char buffer[4096];
    while (request.find("\r\n\r\n") == std::string::npos) {
        const auto count = recv(client, buffer, sizeof(buffer), 0);
        if (count <= 0) {
            return {};
        }
        request.append(buffer, static_cast<std::size_t>(count));
    }
    return request;
}

void respond(
    socket_t client,
    int status,
    const std::string &reason,
    const std::string &body,
    const std::string &extra_header = "")
{
    std::ostringstream response;
    response << "HTTP/1.1 " << status << ' ' << reason << "\r\n"
             << "Content-Type: application/json\r\n"
             << "Content-Length: " << body.size() << "\r\n";
    if (!extra_header.empty()) {
        response << extra_header << "\r\n";
    }
    response << "Connection: close\r\n\r\n" << body;
    send_all(client, response.str());
}

std::string extract_request_id(const std::string &request)
{
    std::smatch match;
    const std::regex pattern(
        "X-Request-ID: ([^\\r\\n]+)",
        std::regex::icase);
    return std::regex_search(request, match, pattern)
        ? match[1].str()
        : "missing";
}

std::string extract_scenario(const std::string &path)
{
    const std::string marker = "scenario=";
    const auto start = path.find(marker);
    return start == std::string::npos
        ? "ok"
        : path.substr(start + marker.size());
}

void handle(socket_t client)
{
    const std::string request = receive_request(client);
    if (request.empty()) {
        return;
    }

    std::istringstream first_line(request);
    std::string method;
    std::string path;
    first_line >> method >> path;

    if (method == "GET" && path == "/health") {
        respond(client, 200, "OK", "{\"status\":\"ok\"}");
        return;
    }
    if (method != "GET" || path.find("/api/config") != 0) {
        respond(
            client,
            404,
            "Not Found",
            "{\"code\":\"not_found\",\"message\":\"Unknown resource\"}");
        return;
    }

    const std::string scenario = extract_scenario(path);
    const std::string request_id = extract_request_id(request);
    const std::string key = scenario + '|' + request_id;
    const int attempt = ++attempts[key];

    std::cout << "request_id=" << request_id
              << " scenario=" << scenario
              << " attempt=" << attempt << std::endl;

    if (scenario == "bad-request") {
        respond(
            client,
            400,
            "Bad Request",
            "{\"code\":\"invalid_request\",\"message\":\"Request is invalid\"}");
    } else if (scenario == "unauthorized") {
        respond(
            client,
            401,
            "Unauthorized",
            "{\"code\":\"unauthorized\",\"message\":\"Authentication required\"}");
    } else if (scenario == "rate-limit" && attempt == 1) {
        respond(
            client,
            429,
            "Too Many Requests",
            "{\"code\":\"rate_limited\",\"message\":\"Try later\"}",
            "Retry-After: 1");
    } else if (scenario == "flaky" && attempt == 1) {
        respond(
            client,
            500,
            "Internal Server Error",
            "{\"code\":\"temporary_failure\",\"message\":\"Try again\"}");
    } else if (scenario == "bad-json") {
        respond(
            client,
            200,
            "OK",
            "{\"sensor_id\":\"temperature-1\", broken");
    } else if (scenario == "wrong-type") {
        respond(
            client,
            200,
            "OK",
            "{\"sensor_id\":\"temperature-1\","
            "\"value\":\"warm\",\"unit\":\"C\"}");
    } else if (scenario == "slow") {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        respond(
            client,
            200,
            "OK",
            "{\"sensor_id\":\"temperature-1\","
            "\"value\":21.5,\"unit\":\"C\"}");
    } else {
        respond(
            client,
            200,
            "OK",
            "{\"sensor_id\":\"temperature-1\","
            "\"value\":21.5,\"unit\":\"C\"}");
    }
}

int main(int argc, char **argv)
{
    int port = 8093;
    for (int index = 1; index < argc; ++index) {
        if (std::string(argv[index]) == "--port" && index + 1 < argc) {
            port = std::stoi(argv[++index]);
        }
    }

    try {
        [[maybe_unused]] SocketRuntime runtime;
        const socket_t server = socket(AF_INET, SOCK_STREAM, 0);
        if (server == invalid_socket) {
            throw std::runtime_error("socket failed");
        }

        int reuse = 1;
        setsockopt(
            server,
            SOL_SOCKET,
            SO_REUSEADDR,
            reinterpret_cast<const char *>(&reuse),
            sizeof(reuse));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<unsigned short>(port));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (bind(
                server,
                reinterpret_cast<sockaddr *>(&address),
                sizeof(address)) != 0
            || listen(server, 16) != 0) {
            throw std::runtime_error("bind/listen failed");
        }

        std::signal(SIGINT, [](int) { running = false; });
        std::cout << "Server: http://127.0.0.1:" << port << std::endl;
        while (running) {
            sockaddr_in peer{};
#ifdef _WIN32
            int size = sizeof(peer);
#else
            socklen_t size = sizeof(peer);
#endif
            const socket_t client = accept(
                server,
                reinterpret_cast<sockaddr *>(&peer),
                &size);
            if (client == invalid_socket) {
                continue;
            }
            handle(client);
            close_socket(client);
        }
        close_socket(server);
    } catch (const std::exception &error) {
        std::cerr << "SERVER ERROR: " << error.what() << '\n';
        return 1;
    }
}
