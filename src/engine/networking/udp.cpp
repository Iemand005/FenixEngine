#include <iostream>
#include <string>
#include <cstring>
#include <system_error>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#define SOCKET_ERRNO WSAGetLastError()
using socket_t = SOCKET;
#endif

void test()
{

#ifdef _WIN32
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    std::cerr << "WSAStartup failed\n";
    return;
  }
#endif
  socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == INVALID_SOCKET)
  {
    std::cerr << "Socket creation failed: " << SOCKET_ERRNO << "\n";
    return;
  }

  sockaddr_in receiverAddr{};
  receiverAddr.sin_family = AF_INET;
  receiverAddr.sin_port = htons(8888);

  if (inet_pton(AF_INET, "127.0.0.1", &receiverAddr.sin_addr) <= 0)
  {
    std::cerr << "Invalid address\n";
    CLOSE_SOCKET(sock);
    return;
  }

  std::string message = "hello";
  size_t sent = sendto(sock, message.c_str(), message.size(), 0,
                        (sockaddr *)&receiverAddr, sizeof(receiverAddr));

  if (sent < 0)
  {
    std::cerr << "Send failed: " << SOCKET_ERRNO << "\n";
  }
  else
  {
    std::cout << "Sent " << sent << " bytes: " << message << "\n";
  }

  // 4. Cleanup
  CLOSE_SOCKET(sock);
#ifdef _WIN32
  WSACleanup();
#endif
}