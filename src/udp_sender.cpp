#include "socket_common.hpp"
#include <string>

int main() {
    SocketRuntime runtime;
    if (!runtime.ok()) {
        print_socket_error("socket runtime");
        return 1;
    }

    Socket sender = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sender == INVALID_SOCKET_VALUE) {
        print_socket_error("socket");
        return 1;
    }

    sockaddr_in receiver{};
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(5001);
    inet_pton(AF_INET, "127.0.0.1", &receiver.sin_addr);

    const std::string payload =
        R"({"sensorId":"temp-demo-01","value":21.7,"unit":"C"})";
    int sent = sendto(
        sender, payload.data(), static_cast<int>(payload.size()), 0,
        reinterpret_cast<sockaddr*>(&receiver), sizeof(receiver));

    if (sent == SOCKET_CALL_ERROR) {
        print_socket_error("sendto");
    } else {
        std::cout << "Skickade " << sent << " byte\n";
    }

    close_socket(sender);
    return 0;
}
