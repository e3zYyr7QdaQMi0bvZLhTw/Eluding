#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

namespace evades {

class UDPSocket {
public:
    UDPSocket() : m_socket(
#ifdef _WIN32
        INVALID_SOCKET
#else
        -1
#endif
    ) {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("Failed to initialize Winsock");
        }
#endif

        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (
#ifdef _WIN32
            m_socket == INVALID_SOCKET
#else
            m_socket == -1
#endif
        ) {
            throw std::runtime_error("Failed to create socket");
        }
    }

    ~UDPSocket() {
        if (
#ifdef _WIN32
            m_socket != INVALID_SOCKET
#else
            m_socket != -1
#endif
        ) {
#ifdef _WIN32
            closesocket(m_socket);
            WSACleanup();
#else
            close(m_socket);
#endif
        }
    }

    void bind(int port) {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (::bind(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == 
#ifdef _WIN32
            SOCKET_ERROR
#else
            -1
#endif
        ) {
            throw std::runtime_error("Failed to bind socket");
        }
    }

    void setNonBlocking(bool nonBlocking) {
#ifdef _WIN32
        u_long mode = nonBlocking ? 1 : 0;
        if (ioctlsocket(m_socket, FIONBIO, &mode) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to set non-blocking mode");
        }
#else
        int flags = fcntl(m_socket, F_GETFL, 0);
        if (flags == -1) {
            throw std::runtime_error("Failed to get socket flags");
        }

        if (nonBlocking) {
            flags |= O_NONBLOCK;
        } else {
            flags &= ~O_NONBLOCK;
        }

        if (fcntl(m_socket, F_SETFL, flags) == -1) {
            throw std::runtime_error("Failed to set non-blocking mode");
        }
#endif
    }

    size_t sendTo(const std::vector<uint8_t>& data, const std::string& address, int port) {
        sockaddr_in destAddr;
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(port);

#ifdef _WIN32
        inet_pton(AF_INET, address.c_str(), &destAddr.sin_addr);
#else
        destAddr.sin_addr.s_addr = inet_addr(address.c_str());
#endif

        int sent = sendto(m_socket, reinterpret_cast<const char*>(data.data()), 
                         static_cast<int>(data.size()), 0, 
                         reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));

        if (sent == 
#ifdef _WIN32
            SOCKET_ERROR
#else
            -1
#endif
        ) {
            return 0;
        }

        return static_cast<size_t>(sent);
    }

    size_t receiveFrom(std::vector<uint8_t>& buffer, std::string& fromAddress, int& fromPort) {
        buffer.resize(524288);

        sockaddr_in senderAddr;
        socklen_t senderAddrSize = sizeof(senderAddr);

        int received = recvfrom(m_socket, reinterpret_cast<char*>(buffer.data()), 
                               static_cast<int>(buffer.size()), 0, 
                               reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrSize);

        if (received == 
#ifdef _WIN32
            SOCKET_ERROR
#else
            -1
#endif
        ) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK || error == WSAECONNRESET) {
                return 0;
            }
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return 0;
            }
#endif
            return 0;
        }

        buffer.resize(received);

        char addrBuffer[INET_ADDRSTRLEN];
#ifdef _WIN32
        inet_ntop(AF_INET, &senderAddr.sin_addr, addrBuffer, INET_ADDRSTRLEN);
#else
        inet_ntop(AF_INET, &(senderAddr.sin_addr), addrBuffer, INET_ADDRSTRLEN);
#endif
        fromAddress = addrBuffer;
        fromPort = ntohs(senderAddr.sin_port);

        return static_cast<size_t>(received);
    }

private:
#ifdef _WIN32
    SOCKET m_socket;
#else
    int m_socket;
#endif
};

} 