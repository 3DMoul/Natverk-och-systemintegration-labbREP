#include "net.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>

using Clock = std::chrono::steady_clock;
std::atomic<bool> running{true};
unsigned long long requests = 0, accepted = 0, validation_errors = 0, server_errors = 0;
double processing_sum = 0.0, processing_max = 0.0;
int delay_ms = 0, failure_every = 0;

std::string timestamp()
{
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::string receive_all_headers(socket_t client)
{
    std::string raw;
    char buffer[4096];
    while (raw.find("\r\n\r\n") == std::string::npos) {
        const auto count = recv(client, buffer, sizeof(buffer), 0);
        if (count <= 0) {
            return {};
        }
        raw.append(buffer, static_cast<std::size_t>(count));
    }
    const auto header_end = raw.find("\r\n\r\n");
    std::smatch match;
    const std::regex length_re(
        "Content-Length: ([0-9]+)",
        std::regex::icase);
    const std::size_t length = std::regex_search(raw, match, length_re)
        ? std::stoul(match[1].str())
        : 0;
    while (raw.size() - header_end - 4 < length) {
        const auto count = recv(client, buffer, sizeof(buffer), 0);
        if (count <= 0) {
            break;
        }
        raw.append(buffer, static_cast<std::size_t>(count));
    }
    return raw;
}

void respond(socket_t client, int status, const std::string &reason, const std::string &body)
{
    std::ostringstream out;
    out << "HTTP/1.1 " << status << ' ' << reason << "\r\n"
        << "Content-Type: application/json\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << body;
    send_all(client, out.str());
}

void handle(socket_t client)
{
    const std::string raw = receive_all_headers(client);
    if (raw.empty()) {
        return;
    }

    std::istringstream first(raw);
    std::string method;
    std::string path;
    first >> method >> path;

    if (method == "GET" && path == "/health") {
        respond(client, 200, "OK", "{\"status\":\"ok\"}");
        return;
    }
    if (method == "GET" && path == "/metrics") {
        const double average = requests ? processing_sum / requests : 0.0;
        std::ostringstream body;
        body << std::fixed << std::setprecision(3)
             << "{\"requests_total\":" << requests
             << ",\"accepted_total\":" << accepted
             << ",\"validation_errors_total\":" << validation_errors
             << ",\"server_errors_total\":" << server_errors
             << ",\"processing_ms_avg\":" << average
             << ",\"processing_ms_max\":" << processing_max
             << '}';
        respond(client, 200, "OK", body.str());
        return;
    }

    if (method != "POST" || path != "/api/readings") {
        respond(client, 404, "Not Found", "{\"error\":\"not_found\"}");
        return;
    }

    const auto start = Clock::now();
    ++requests;

    std::smatch id_match;
    const std::regex id_re(
        "X-Request-ID: ([^\\r\\n]+)",
        std::regex::icase);
    const std::string request_id = std::regex_search(raw, id_match, id_re)
        ? id_match[1].str()
        : "server-" + std::to_string(requests);

    const auto body_pos = raw.find("\r\n\r\n");
    const std::string body = body_pos == std::string::npos
        ? ""
        : raw.substr(body_pos + 4);

    int status = 202;
    std::string event = "accepted";
    const std::regex valid_value(
        "\"value\"\\s*:\\s*-?[0-9]+(?:\\.[0-9]+)?");

    if (!std::regex_search(body, valid_value)) {
        status = 400;
        event = "value_must_be_finite_number";
        ++validation_errors;
    } else {
        if (delay_ms != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }

        const bool should_fail = failure_every != 0
            && requests % static_cast<unsigned>(failure_every) == 0;
        if (should_fail) {
            status = 500;
            event = "simulated_server_failure";
            ++server_errors;
        } else {
            ++accepted;
        }
    }

    const double duration =
        std::chrono::duration<double, std::milli>(Clock::now() - start)
            .count();
    processing_sum += duration;
    processing_max = std::max(processing_max, duration);

    std::cout << "{\"timestamp\":\"" << timestamp()
              << "\",\"event\":\"" << event
              << "\",\"request_id\":\"" << request_id
              << "\",\"status\":" << status
              << ",\"processing_ms\":" << std::fixed
              << std::setprecision(3) << duration
              << "}" << std::endl;

    std::ostringstream response_body;
    response_body << "{\"event\":\"" << event
                  << "\",\"request_id\":\"" << request_id
                  << "\",\"processing_ms\":" << duration
                  << '}';

    std::string reason = "Internal Server Error";
    if (status == 202) {
        reason = "Accepted";
    } else if (status == 400) {
        reason = "Bad Request";
    }
    respond(client, status, reason, response_body.str());
}

int main(int argc, char **argv)
{
    int port = 8091;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--port" && index + 1 < argc) {
            port = std::stoi(argv[++index]);
        } else if (argument == "--delay-ms" && index + 1 < argc) {
            delay_ms = std::stoi(argv[++index]);
        } else if (argument == "--failure-every" && index + 1 < argc) {
            failure_every = std::stoi(argv[++index]);
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

        const bool bind_failed =
            bind(
                server,
                reinterpret_cast<sockaddr *>(&address),
                sizeof(address)) != 0;
        if (bind_failed || listen(server, 16) != 0) {
            throw std::runtime_error("bind/listen failed");
        }

        std::signal(SIGINT, [](int) { running = false; });
        std::cout << "Server: http://127.0.0.1:" << port
                  << " delay_ms=" << delay_ms
                  << " failure_every=" << failure_every
                  << std::endl;

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
