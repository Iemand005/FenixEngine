
#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <system_error>

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#define SOCKET_ERRNO WSAGetLastError()
using socket_t = SOCKET;

typedef void (* UDPResponseHandler)(const char* data, size_t size);

const int port = 2130;

class UDPClient
{
  socket_t sock;
public:

  void openSocket()
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
  }

  void send(const char *packet, size_t size)
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

    // std::string message = "hello";

    sockaddr_in receiverAddr{};
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &receiverAddr.sin_addr) <= 0)
    {
      std::cerr << "Invalid address\n";
      this->closeSocket();
      return;
    }
    size_t sent = sendto(sock, packet, size, 0, (sockaddr *)&receiverAddr, sizeof(receiverAddr));

    if (sent >= 0)
    {
      std::cout << "Sent " << sent << " bytes: " << (char*)packet << "\n";
    }
    else
    {
      std::cerr << "Send failed: " << SOCKET_ERRNO << "\n";
    }

    this->closeSocket();
  }

  void closeSocket()
  {
    CLOSE_SOCKET(sock);
    WSACleanup();
  }
};

class UDPServer {
  socket_t sock;
public:

UDPServer() {
  init();
}

int init() {
  return 0;
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    std::cerr << "WSAStartup failed\n";
    return 1;
  }

  socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == INVALID_SOCKET)
  {
    std::cerr << "Socket creation failed: " << SOCKET_ERRNO << "\n";
    return 1;
  }
  return 0;
}

int startListening(UDPResponseHandler callback)
{
    WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    std::cerr << "WSAStartup failed\n";
    return 1;
  }

  socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
  

  while (true) {

  // 3. Receive data
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

      callback(buffer, received);
    }
    else
    {
      std::cerr << "Receive failed: " << SOCKET_ERRNO << "\n";
    }
  }

  // 4. Cleanup
  CLOSE_SOCKET(sock);
#ifdef _WIN32
  WSACleanup();
#endif

  return 0;
}
};