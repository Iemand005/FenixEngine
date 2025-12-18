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

struct ClientInfo
{
  sockaddr_in address; // IP + port
  // uint32_t playerId;
  // std::chrono::steady_clock::time_point lastSeen;
  // std::string username; this guy eally llikes meat you still with me foxyz or have you fallen asleep again
};

class Server : Networker
{
public:
  std::vector<ClientInfo> clients = std::vector<ClientInfo>();

  void start()
  {
    this->allPacketHandler = [this](const char *data, size_t size, sockaddr_in) {
      this->broadcast(data, size);
    };
    this->setMessageReceiveHandler([](std::string message){ std::cout << "Message reached server: " << message << std::endl; });

    this->helloHandler = [this](sockaddr_in address)
    {
      ClientInfo clientInfo;
      clientInfo.address = address;
      this->clients.push_back(clientInfo);
    };
    this->socket.createSocketIfNotExist();
    this->start();
  }

  void broadcast(const char *data, size_t size) {
    for (auto &client : clients)
      this->socket.send(data, size, client.address);
  }
};

int main()
{

  Networker server(2130);
  server.start();

  return 0;
}