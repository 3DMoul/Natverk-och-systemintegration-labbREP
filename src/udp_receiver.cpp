#include "socket_common.hpp"
#include <string>

int main() {
    SocketRuntime runtime;
    if (!runtime.ok()) {
        print_socket_error("socket runtime");
        return 1;
    }

    Socket receiver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiver == INVALID_SOCKET_VALUE) {
        print_socket_error("socket");
        return 1;
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(5001);
    inet_pton(AF_INET, "127.0.0.1", &local.sin_addr);

    if (bind(receiver, reinterpret_cast<sockaddr*>(&local),
             sizeof(local)) == SOCKET_CALL_ERROR) {
        print_socket_error("bind");
        close_socket(receiver);
        return 1;
    }

    std::cout << "UDP-mottagare lyssnar på 127.0.0.1:5001\n";
    sockaddr_in sender{};
    SocketLength senderLength = sizeof(sender);
    char buffer[4096]{};
    int received = recvfrom(
        receiver, buffer, sizeof(buffer), 0,
        reinterpret_cast<sockaddr*>(&sender), &senderLength);

    if (received > 0) {
        std::cout << "Mottaget: " << std::string(buffer, received) << '\n';
    } else if (received == SOCKET_CALL_ERROR) {
        print_socket_error("recvfrom");
    }

    close_socket(receiver);
    return 0;
}
