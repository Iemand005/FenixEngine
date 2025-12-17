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

class UDP {
  socket_t sock;

  void openSocket() {
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

void send(const char* packet, size_t size)
{
  std::string message = "hello";

    sockaddr_in receiverAddr{};
  receiverAddr.sin_family = AF_INET;
  receiverAddr.sin_port = htons(8888);

  if (inet_pton(AF_INET, "127.0.0.1", &receiverAddr.sin_addr) <= 0)
  {
    std::cerr << "Invalid address\n";
    this->closeSocket();
    return;
  }
  size_t sent = sendto(sock, packet, size, 0, (sockaddr *)&receiverAddr, sizeof(receiverAddr));

  if (sent >= 0)
  {
    std::cout << "Sent " << sent << " bytes: " << message << "\n";
  }
  else
  {
    std::cerr << "Send failed: " << SOCKET_ERRNO << "\n";
  }

  this->closeSocket();  
}

void closeSocket() {
CLOSE_SOCKET(sock);
  WSACleanup();
}
};
