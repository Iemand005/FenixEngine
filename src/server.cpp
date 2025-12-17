#include <iostream>
#include <string>
#include <cstring>
#include <system_error>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#define SOCKET_ERRNO WSAGetLastError()
using socket_t = SOCKET;

#include "engine/networking/networking.hpp"

int main()
{

  NetworkerServer server;

  server.start();


// #ifdef _WIN32
//   WSADATA wsaData;
//   if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
//   {
//     std::cerr << "WSAStartup failed\n";
//     return 1;
//   }
// #endif

//   // 1. Create UDP socket
//   socket_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//   if (sock == INVALID_SOCKET)
//   {
//     std::cerr << "Socket creation failed: " << SOCKET_ERRNO << "\n";
//     return 1;
//   }

//   // 2. Bind to local address (all interfaces, port 8888)
//   sockaddr_in localAddr{};
//   localAddr.sin_family = AF_INET;
//   localAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
//   localAddr.sin_port = htons(8888);

//   if (bind(sock, (sockaddr *)&localAddr, sizeof(localAddr)) < 0)
//   {
//     std::cerr << "Bind failed: " << SOCKET_ERRNO << "\n";
//     CLOSE_SOCKET(sock);
//     return 1;
//   }

//   std::cout << "Listening on UDP port 8888...\n";
//   while (true) {

//   // 3. Receive data
//   char buffer[1024];
//   sockaddr_in senderAddr{};
//   socklen_t senderLen = sizeof(senderAddr);

//   size_t received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&senderAddr, &senderLen);

//     if (received >= 0)
//     {
//       buffer[received] = '\0';
//       char senderIP[INET_ADDRSTRLEN];
//       inet_ntop(AF_INET, &senderAddr.sin_addr, senderIP, sizeof(senderIP));
  
//       std::cout << "Received " << received << " bytes from " << senderIP << ":" << ntohs(senderAddr.sin_port) << " - " << buffer << "\n";
//     }
//     else
//     {
//       std::cerr << "Receive failed: " << SOCKET_ERRNO << "\n";
//     }
//   }

//   // 4. Cleanup
//   CLOSE_SOCKET(sock);
// #ifdef _WIN32
//   WSACleanup();
// #endif

  return 0;
}