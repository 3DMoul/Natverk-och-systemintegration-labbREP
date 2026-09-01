#pragma once

#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using Socket = SOCKET;
using SocketLength = int;
constexpr Socket INVALID_SOCKET_VALUE = INVALID_SOCKET;
constexpr int SOCKET_CALL_ERROR = SOCKET_ERROR;
#else
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
using Socket = int;
using SocketLength = socklen_t;
constexpr Socket INVALID_SOCKET_VALUE = -1;
constexpr int SOCKET_CALL_ERROR = -1;
#endif

class SocketRuntime {
public:
    SocketRuntime() {
#ifdef _WIN32
        WSADATA data{};
        ok_ = WSAStartup(MAKEWORD(2, 2), &data) == 0;
#endif
    }

    ~SocketRuntime() {
#ifdef _WIN32
        if (ok_) {
            WSACleanup();
        }
#endif
    }

    bool ok() const { return ok_; }

private:
    bool ok_ = true;
};

inline void close_socket(Socket socketHandle) {
#ifdef _WIN32
    closesocket(socketHandle);
#else
    close(socketHandle);
#endif
}

inline void print_socket_error(const char* operation) {
    std::cerr << operation << " misslyckades: ";
#ifdef _WIN32
    std::cerr << WSAGetLastError();
#else
    std::cerr << std::strerror(errno);
#endif
    std::cerr << '\n';
}
