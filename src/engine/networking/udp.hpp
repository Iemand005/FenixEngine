
#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <system_error>
#include <functional>

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#define SOCKET_ERRNO WSAGetLastError()

typedef void (*UDPResponseHandler)(const char *data, size_t size);

// const int PORT = 2130;

class UDPSocket
{
  SOCKET sock = INVALID_SOCKET;
  using ReceiveCallback = std::function<void(const char *data, size_t size, const sockaddr_in &from)>;

public:
  UDPSocket() {}

  template <typename T>
  void send(unsigned short port, std::string address = "127.0.0.1") {
    T packet;
    this->send<T>(packet, port, address);
  }

  template <typename T>
  void send(sockaddr_in address) {
    T packet;
    this->send<T>(packet, address);
  }

  template <typename T>
  void send(T packet, unsigned short port, std::string address = "127.0.0.1") {
    this->send((char*)&packet, sizeof(T), port, address);
  }

  template <typename T>
  void send(T packet, sockaddr_in address) {
    this->send((char*)&packet, sizeof(T), address);
  }

  void send(const char *packet, size_t size, unsigned short port, std::string address = "127.0.0.1") {
    sockaddr_in receiverAddr{};
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, address.c_str(), &receiverAddr.sin_addr) <= 0)
    {
      std::cerr << "Invalid address\n";
      this->close();
      return;
    }
    this->send(packet, size, receiverAddr);
  }

  void close() {
    std::cerr << "Closing socket...\n";
    CLOSE_SOCKET(sock);
    sock = INVALID_SOCKET;
    WSACleanup();
  }

  void send(const char *packet, size_t size, sockaddr_in address)
  {

    this->createSocketIfNotExist();
    
    size_t sent = sendto(sock, packet, size, 0, (sockaddr *)&address, sizeof(address));

    if (sent >= 0)
    {
      char senderIP[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &address.sin_addr, senderIP, sizeof(senderIP));

      std::cout << "Sent " << sent << " bytes: " << (char *)packet <<" To " << senderIP << ":" << ntohs(address.sin_port) <<"\n";
    }
    else
    {
      std::cerr << "Send failed: " << SOCKET_ERRNO << "\n";
    }
  }

  bool createSocketIfNotExist() {
    // if (sock != INVALID_SOCKET) return false;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
      std::cerr << "WSAStartup failed\n";
      return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
      std::cerr << "Socket creation failed: " << SOCKET_ERRNO << "\n";
      return false;
    }


    this->sock = sock;

    return true;
  }

  bool bindSocket() {
    bindSocket(0);
  }

  bool bindSocket(unsigned short port) {
    this->createSocketIfNotExist();

    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    localAddr.sin_port = htons(port);

    if (bind(sock, (sockaddr *)&localAddr, sizeof(localAddr)) < 0)
    {
      std::cerr << "Bind failed: " << SOCKET_ERRNO << "\n";
      CLOSE_SOCKET(sock);
      return false;
    }
    return true;
  }

  int startListening(unsigned short port, ReceiveCallback callback)
  {
    // this->createSocketIfNotExist();
    if (port) this->bindSocket(port);
    std::cout << "Listening on UDP port " << port << "..." << std::endl;

    // Blocking mode
    // u_long mode = 1;
    // ioctlsocket(sock, FIONBIO, &mode);

    while (true)
    {

      char buffer[1024];
      sockaddr_in senderAddr{};
      socklen_t senderLen = sizeof(senderAddr);
    std::cout << "Waiting for next packet..." << std::endl;

      int received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&senderAddr, &senderLen);
     
      if (received >= 0)
      {
        buffer[received] = '\0';
        char senderIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &senderAddr.sin_addr, senderIP, sizeof(senderIP));

        std::cout << "Received " << received << " bytes from " << senderIP << ":" << ntohs(senderAddr.sin_port) << " - " << buffer << std::endl;

        callback(buffer, received, senderAddr);
      }
      else
      {
        std::cerr << "Receive failed: " << SOCKET_ERRNO << "\n";
        this->close();
        // this->createSocketIfNotExist();
        this->bindSocket(port);
      }
    }

    this->close();

    return 0;
  }
};