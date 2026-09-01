#include "socket_common.hpp"
#include <string>

int main() {
    SocketRuntime runtime;
    if (!runtime.ok()) {
        print_socket_error("socket runtime");
        return 1;
    }

    Socket listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET_VALUE) {
        print_socket_error("socket");
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (bind(listener, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) == SOCKET_CALL_ERROR ||
        listen(listener, SOMAXCONN) == SOCKET_CALL_ERROR) {
        print_socket_error("bind/listen");
        close_socket(listener);
        return 1;
    }

    std::cout << "TCP-server lyssnar på 127.0.0.1:5000\n";
    Socket client = accept(listener, nullptr, nullptr);
    if (client == INVALID_SOCKET_VALUE) {
        print_socket_error("accept");
        close_socket(listener);
        return 1;
    }

    char buffer[4096]{};
    int received = recv(client, buffer, sizeof(buffer), 0);
    if (received > 0) {
        std::cout << "Mottaget: " << std::string(buffer, received) << '\n';
        const std::string response = R"({"status":"ok"})";
        send(client, response.data(),
             static_cast<int>(response.size()), 0);
    } else if (received == SOCKET_CALL_ERROR) {
        print_socket_error("recv");
    }

    close_socket(client);
    close_socket(listener);
    return 0;
}
