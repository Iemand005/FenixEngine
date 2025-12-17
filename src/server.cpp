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

  server.setMessageReceiveHandler([](std::string message) {
    std::cout << "Message reached server: " << message << std::endl;
  });

  server.start();

  return 0;
}