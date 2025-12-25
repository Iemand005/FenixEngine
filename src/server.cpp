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
    server.allPacketHandler = [this](const char *data, size_t size, PacketType type, sockaddr_in sender) {
      // if (type != PacketType::Hello)
      //   server.broadcast(data, size);
    };
    server.setMessageReceiveHandler([](std::string message, ClientData sender){
      std::cout << "Message reached server: " << message << " with client ID: " << (int)sender.id << " and username: " << sender.username << std::endl;
    });

    // server.socket.createSocketIfNotExist();
    server.startServer();
  }

  
};

int main()
{
  Server server;
  server.start();

  return 0;
}