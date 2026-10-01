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
inline void set_receive_timeout(socket_t socket, int milliseconds)
{
    const DWORD timeout = static_cast<DWORD>(milliseconds);
    setsockopt(
        socket,
        SOL_SOCKET,
        SO_RCVTIMEO,
        reinterpret_cast<const char *>(&timeout),
        sizeof(timeout));
}
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
#include <sys/time.h>
#include <unistd.h>
using socket_t = int;
constexpr socket_t invalid_socket = -1;
inline void close_socket(socket_t value) { close(value); }
inline void set_receive_timeout(socket_t socket, int milliseconds)
{
    timeval timeout{};
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}
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
#ifdef _WIN32
            0);
#else
            MSG_NOSIGNAL);
#endif
        if (result <= 0) {
            throw std::runtime_error("socket send failed");
        }
        sent += static_cast<std::size_t>(result);
    }
}
