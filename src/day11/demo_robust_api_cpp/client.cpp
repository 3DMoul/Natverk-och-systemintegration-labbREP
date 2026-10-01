#include "net.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>

struct Response {
    int status = 0;
    std::string headers;
    std::string body;
};

Response request_once(
    const std::string &host,
    int port,
    const std::string &scenario,
    int timeout_ms)
{
    Response result;
    const socket_t socket_handle = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_handle == invalid_socket) {
        return result;
    }
    set_receive_timeout(socket_handle, timeout_ms);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<unsigned short>(port));
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1
        || connect(
               socket_handle,
               reinterpret_cast<sockaddr *>(&address),
               sizeof(address)) != 0) {
        close_socket(socket_handle);
        return result;
    }

    const std::string request_id = "day11-" + scenario;
    std::ostringstream request;
    request << "GET /api/config?scenario=" << scenario << " HTTP/1.1\r\n"
            << "Host: " << host << ':' << port << "\r\n"
            << "Accept: application/json\r\n"
            << "Authorization: Bearer classroom-placeholder\r\n"
            << "X-Request-ID: " << request_id << "\r\n"
            << "Connection: close\r\n\r\n";
    send_all(socket_handle, request.str());

    std::string raw;
    char buffer[4096];
    while (true) {
        const auto count = recv(socket_handle, buffer, sizeof(buffer), 0);
        if (count <= 0) {
            break;
        }
        raw.append(buffer, static_cast<std::size_t>(count));
    }
    close_socket(socket_handle);
    if (raw.empty()) {
        return result;
    }

    std::istringstream status_line(raw);
    std::string protocol;
    status_line >> protocol >> result.status;
    const auto separator = raw.find("\r\n\r\n");
    if (separator != std::string::npos) {
        result.headers = raw.substr(0, separator);
        result.body = raw.substr(separator + 4);
    }
    return result;
}

bool valid_config(const std::string &body)
{
    const std::regex sensor("\"sensor_id\"\\s*:\\s*\"[^\"]+\"");
    const std::regex value("\"value\"\\s*:\\s*-?[0-9]+(?:\\.[0-9]+)?");
    const std::regex unit("\"unit\"\\s*:\\s*\"[^\"]+\"");
    return std::regex_search(body, sensor)
        && std::regex_search(body, value)
        && std::regex_search(body, unit)
        && !body.empty()
        && body.front() == '{'
        && body.back() == '}';
}

bool retryable(int status)
{
    return status == 429
        || status == 500
        || status == 502
        || status == 503
        || status == 504;
}

int retry_after_seconds(const std::string &headers)
{
    std::smatch match;
    const std::regex pattern("Retry-After: ([0-9]+)", std::regex::icase);
    return std::regex_search(headers, match, pattern)
        ? std::stoi(match[1].str())
        : 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: troubleshooting_client SCENARIO [--port N]\n";
        return 2;
    }

    const std::string scenario = argv[1];
    std::string host = "127.0.0.1";
    int port = 8093;
    int timeout_ms = 400;
    int max_attempts = 3;
    for (int index = 2; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--host" && index + 1 < argc) {
            host = argv[++index];
        } else if (argument == "--port" && index + 1 < argc) {
            port = std::stoi(argv[++index]);
        } else if (argument == "--timeout-ms" && index + 1 < argc) {
            timeout_ms = std::stoi(argv[++index]);
        } else if (argument == "--max-attempts" && index + 1 < argc) {
            max_attempts = std::stoi(argv[++index]);
        }
    }

    [[maybe_unused]] SocketRuntime runtime;
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        const Response response =
            request_once(host, port, scenario, timeout_ms);

        if (response.status == 200) {
            if (!valid_config(response.body)) {
                std::cout << "attempt=" << attempt
                          << " error=contract decision=stop\n";
                return 1;
            }
            std::cout << "attempt=" << attempt
                      << " status=200 decision=success\n";
            return 0;
        }

        if (response.status == 0) {
            std::cout << "attempt=" << attempt
                      << " error=timeout_or_connection decision=stop\n";
            return 1;
        }

        const bool should_retry = retryable(response.status)
            && attempt < max_attempts;
        std::cout << "attempt=" << attempt
                  << " status=" << response.status
                  << " decision=" << (should_retry ? "retry" : "stop")
                  << '\n';
        if (!should_retry) {
            return 1;
        }

        const int retry_after = retry_after_seconds(response.headers);
        const int wait_ms = retry_after > 0
            ? std::min(retry_after * 1000, 2000)
            : 100 * (1 << (attempt - 1));
        std::cout << "wait_ms=" << wait_ms << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
    }
    return 1;
}
