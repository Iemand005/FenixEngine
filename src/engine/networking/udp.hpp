
#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <system_error>
#include <functional>
#include <thread>

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#define SOCKET_ERRNO WSAGetLastError()

typedef void (*UDPResponseHandler)(const char *data, size_t size);

const int port = 2130;

class UDPSocket
{
  SOCKET sock;
  using ReceiveCallback = std::function<void(const char *data, size_t size, const sockaddr_in &from)>;

public:
  UDPSocket()
  {
  }

  void send(const char *packet, size_t size, char* address = "127.0.0.1") {
    sockaddr_in receiverAddr{};
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, address, &receiverAddr.sin_addr) <= 0)
    {
      std::cerr << "Invalid address\n";
      this->close();
      return;
    }
    this->send(packet, size, receiverAddr);
  }

  void close() {
    CLOSE_SOCKET(sock);
    sock = 0;
    WSACleanup();
  }

  void send(const char *packet, size_t size, sockaddr_in address)
  {

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
      std::cerr << "WSAStartup failed\n";
      return;
    }

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
      std::cerr << "Socket creation failed: " << SOCKET_ERRNO << "\n";
      return;
    }
    
    size_t sent = sendto(sock, packet, size, 0, (sockaddr *)&address, sizeof(address));

    if (sent >= 0)
    {
      std::cout << "Sent " << sent << " bytes: " << (char *)packet << "\n";
    }
    else
    {
      std::cerr << "Send failed: " << SOCKET_ERRNO << "\n";
    }

    // this->closeSocket();
  }

  int startListening(ReceiveCallback callback)
  {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
      std::cerr << "WSAStartup failed\n";
      return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
      std::cerr << "Socket creation failed: " << SOCKET_ERRNO << "\n";
      return 1;
    }
    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    localAddr.sin_port = htons(port);

    if (bind(sock, (sockaddr *)&localAddr, sizeof(localAddr)) < 0)
    {
      std::cerr << "Bind failed: " << SOCKET_ERRNO << "\n";
      CLOSE_SOCKET(sock);
      return 1;
    }
    std::cout << "Listening on UDP port " << port << "..." << std::endl;

    while (true)
    {

      char buffer[1024];
      sockaddr_in senderAddr{};
      socklen_t senderLen = sizeof(senderAddr);

      size_t received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&senderAddr, &senderLen);
      std::cout << "Client said something...";
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
      }
    }

    this->close();

    return 0;
  }

  int startListeningAsync(ReceiveCallback callback) {
    std::thread listenerThread([this, callback](){
      this->startListening(callback);
    });
  }
};