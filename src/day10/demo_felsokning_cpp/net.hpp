#pragma once

#include <stdexcept>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
constexpr socket_t invalid_socket = INVALID_SOCKET;
inline void close_socket(socket_t value) { closesocket(value); }
class SocketRuntime {
public:
    SocketRuntime()
    {
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }

    ~SocketRuntime() { WSACleanup(); }
};
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
constexpr socket_t invalid_socket = -1;
inline void close_socket(socket_t value) { close(value); }
class SocketRuntime {};
#endif

inline void send_all(socket_t socket, const std::string &data)
{
    std::size_t sent = 0;
    while (sent < data.size()) {
        const auto result = send(
            socket,
            data.data() + sent,
            static_cast<int>(data.size() - sent),
            0);
        if (result <= 0) {
            throw std::runtime_error("socket send failed");
        }
        sent += static_cast<std::size_t>(result);
    }
}
