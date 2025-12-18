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

class Server
{
public:

  Networker server;

  Server() {
    server = Networker(2130);
  }

  void start()
  {
    server.allPacketHandler = [this](const char *data, size_t size, sockaddr_in) {
      server.broadcast(data, size);
    };
    server.setMessageReceiveHandler([](std::string message){ std::cout << "Message reached server: " << message << std::endl; });

    server.socket.createSocketIfNotExist();
    server.start();
  }

  
};

int main()
{
  Server server;
  server.start();

  return 0;
}