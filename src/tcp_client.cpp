#include "socket_common.hpp"
#include <string>

int main() {
    SocketRuntime runtime;
    if (!runtime.ok()) {
        print_socket_error("socket runtime");
        return 1;
    }

    Socket client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == INVALID_SOCKET_VALUE) {
        print_socket_error("socket");
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (connect(client, reinterpret_cast<sockaddr*>(&server),
                sizeof(server)) == SOCKET_CALL_ERROR) {
        print_socket_error("connect");
        close_socket(client);
        return 1;
    }

    const std::string payload =
        R"({"sensorId":"temp-demo-01","value":21.7,"unit":"C"})";
    send(client, payload.data(), static_cast<int>(payload.size()), 0);

    char buffer[4096]{};
    int received = recv(client, buffer, sizeof(buffer), 0);
    if (received > 0) {
        std::cout << "Svar: " << std::string(buffer, received) << '\n';
    } else if (received == SOCKET_CALL_ERROR) {
        print_socket_error("recv");
    }

    close_socket(client);
    return 0;
}
