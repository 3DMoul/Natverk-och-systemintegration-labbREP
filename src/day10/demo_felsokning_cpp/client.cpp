#include "net.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

struct Options {
    std::string host = "127.0.0.1";
    int port = 8091;
    int count = 20;
    int interval_ms = 50;
    int invalid_every = 0;
};

Options parse_options(int argc, char **argv)
{
    Options options;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--host" && index + 1 < argc) {
            options.host = argv[++index];
        } else if (argument == "--port" && index + 1 < argc) {
            options.port = std::stoi(argv[++index]);
        } else if (argument == "--count" && index + 1 < argc) {
            options.count = std::stoi(argv[++index]);
        } else if (argument == "--interval-ms" && index + 1 < argc) {
            options.interval_ms = std::stoi(argv[++index]);
        } else if (argument == "--invalid-every" && index + 1 < argc) {
            options.invalid_every = std::stoi(argv[++index]);
        }
    }

    return options;
}

std::string make_body(int number, bool invalid)
{
    if (invalid) {
        return R"({"sensor_id":"sim-1","value":"invalid","unit":"C"})";
    }

    const double value = 20.0 + number / 10.0;
    return "{\"sensor_id\":\"sim-1\",\"value\":"
        + std::to_string(value)
        + ",\"unit\":\"C\"}";
}

int send_reading(
    const Options &options,
    const std::string &request_id,
    const std::string &body)
{
    const socket_t socket_handle = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_handle == invalid_socket) {
        return 0;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<unsigned short>(options.port));

    const bool address_is_valid =
        inet_pton(AF_INET, options.host.c_str(), &address.sin_addr) == 1;
    const bool connected = address_is_valid
        && connect(
               socket_handle,
               reinterpret_cast<sockaddr *>(&address),
               sizeof(address)) == 0;

    if (!connected) {
        close_socket(socket_handle);
        return 0;
    }

    std::ostringstream request;
    request << "POST /api/readings HTTP/1.1\r\n"
            << "Host: " << options.host << ':' << options.port << "\r\n"
            << "Content-Type: application/json\r\n"
            << "X-Request-ID: " << request_id << "\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Connection: close\r\n\r\n"
            << body;
    send_all(socket_handle, request.str());

    std::string response;
    char buffer[4096];
    while (true) {
        const auto received = recv(socket_handle, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            break;
        }
        response.append(buffer, static_cast<std::size_t>(received));
    }
    close_socket(socket_handle);

    std::istringstream input(response);
    std::string protocol;
    int status = 0;
    input >> protocol >> status;
    return status;
}

int main(int argc, char **argv)
{
    const Options options = parse_options(argc, argv);
    [[maybe_unused]] SocketRuntime runtime;

    std::vector<double> durations;
    int successes = 0;
    int http_400 = 0;
    int http_500 = 0;
    int connection_errors = 0;
    const auto run_start = Clock::now();

    for (int number = 1; number <= options.count; ++number) {
        const std::string request_id =
            "day10-" + std::to_string(1000 + number).substr(1);
        const bool invalid = options.invalid_every != 0
            && number % options.invalid_every == 0;
        const std::string body = make_body(number, invalid);

        const auto start = Clock::now();
        const int status = send_reading(options, request_id, body);

        if (status == 202) {
            ++successes;
        } else if (status == 400) {
            ++http_400;
        } else if (status == 500) {
            ++http_500;
        } else if (status == 0) {
            ++connection_errors;
        }

        const double duration =
            std::chrono::duration<double, std::milli>(Clock::now() - start)
                .count();
        durations.push_back(duration);

        std::cout << request_id
                  << " status=" << status
                  << " duration_ms=" << std::fixed << std::setprecision(3)
                  << duration << '\n';

        if (number < options.count) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(options.interval_ms));
        }
    }

    std::sort(durations.begin(), durations.end());
    const double elapsed =
        std::chrono::duration<double>(Clock::now() - run_start).count();
    const double average =
        std::accumulate(durations.begin(), durations.end(), 0.0)
        / durations.size();
    const int nearest_rank = std::max(
        1,
        static_cast<int>(std::ceil(0.95 * durations.size())));
    const std::size_t p95_index =
        static_cast<std::size_t>(nearest_rank - 1);
    const int failures = options.count - successes;

    std::cout << "{\n"
              << "  \"attempts\": " << options.count << ",\n"
              << "  \"successes\": " << successes << ",\n"
              << "  \"failures\": " << failures << ",\n"
              << "  \"error_fraction\": "
              << static_cast<double>(failures) / options.count << ",\n"
              << "  \"throughput_success_per_s\": "
              << successes / elapsed << ",\n"
              << "  \"latency_ms_min\": " << durations.front() << ",\n"
              << "  \"latency_ms_avg\": " << average << ",\n"
              << "  \"latency_ms_p95\": " << durations[p95_index] << ",\n"
              << "  \"latency_ms_max\": " << durations.back() << ",\n"
              << "  \"http_400\": " << http_400 << ",\n"
              << "  \"http_500\": " << http_500 << ",\n"
              << "  \"connection_errors\": " << connection_errors << "\n"
              << "}\n";
}
