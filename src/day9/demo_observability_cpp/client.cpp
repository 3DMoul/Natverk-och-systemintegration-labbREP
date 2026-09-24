#include "net.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Scenario {
    std::string request_id;
    std::string path;
    std::string body;
    int expected;
};

std::string receive_all(socket_t socket)
{
    std::string response;
    char buffer[4096];
    while (true) {
        const auto count = recv(socket, buffer, sizeof(buffer), 0);
        if (count <= 0) break;
        response.append(buffer, static_cast<std::size_t>(count));
    }
    return response;
}

int send_scenario(const std::string &host, int port, const Scenario &scenario)
{
    const socket_t socket_handle = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_handle == invalid_socket) throw std::runtime_error("could not create client socket");
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<unsigned short>(port));
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1 ||
        connect(socket_handle, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
        close_socket(socket_handle);
        throw std::runtime_error("could not connect to " + host + ':' + std::to_string(port));
    }
    std::ostringstream request;
    request << "POST " << scenario.path << " HTTP/1.1\r\n"
            << "Host: " << host << ':' << port << "\r\n"
            << "Content-Type: application/json\r\n"
            << "X-Request-ID: " << scenario.request_id << "\r\n"
            << "Content-Length: " << scenario.body.size() << "\r\n"
            << "Connection: close\r\n\r\n" << scenario.body;
    send_all(socket_handle, request.str());
    const std::string response = receive_all(socket_handle);
    close_socket(socket_handle);

    std::istringstream input(response);
    std::string protocol;
    int status = 0;
    input >> protocol >> status;
    const auto body_start = response.find("\r\n\r\n");
    const std::string body = body_start == std::string::npos ? "" : response.substr(body_start + 4);
    std::string echoed_id;
    const std::string id_header = "X-Request-ID: ";
    if (const auto start = response.find(id_header); start != std::string::npos) {
        const auto value_start = start + id_header.size();
        echoed_id = response.substr(value_start, response.find("\r\n", value_start) - value_start);
    }
    std::cout << "CLIENT request_id=" << scenario.request_id
              << " status=" << status << " echoed_id=" << echoed_id
              << " body=" << body << '\n';
    if (echoed_id != scenario.request_id) throw std::runtime_error("X-Request-ID was not echoed");
    return status;
}

int main(int argc, char **argv)
{
    std::string host = "127.0.0.1";
    int port = 8090;
    for (int index = 1; index + 1 < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--host") host = argv[++index];
        else if (argument == "--port") port = std::stoi(argv[++index]);
    }
    const std::vector<Scenario> scenarios = {
        {"demo-temp-1", "/api/readings", R"({"sensor_id":"temperature-1","value":21.5,"unit":"C"})", 202},
        {"demo-humidity-1", "/api/readings", R"({"sensor_id":"humidity-1","value":47.2,"unit":"%"})", 202},
        {"demo-invalid-1", "/api/readings", R"({"sensor_id":"temperature-1","value":"warm","unit":"C"})", 400},
        {"demo-missing-1", "/api/unknown", R"({"sensor_id":"temperature-1","value":22.0,"unit":"C"})", 404},
    };
    try {
        [[maybe_unused]] SocketRuntime runtime;
        std::vector<int> statuses;
        for (const auto &scenario : scenarios) statuses.push_back(send_scenario(host, port, scenario));
        bool correct = true;
        std::cout << "RESULT statuses=[";
        for (std::size_t index = 0; index < statuses.size(); ++index) {
            if (index) std::cout << ", ";
            std::cout << statuses[index];
            correct = correct && statuses[index] == scenarios[index].expected;
        }
        std::cout << "] expected=[202, 202, 400, 404]\n";
        return correct ? 0 : 1;
    } catch (const std::exception &error) {
        std::cerr << "CLIENT ERROR: " << error.what() << '\n';
        return 1;
    }
}
