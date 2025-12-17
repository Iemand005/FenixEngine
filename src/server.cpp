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

class Server
{
public:
  std::vector<ClientInfo> clients = std::vector<ClientInfo>();

  NetworkerServer server;

  void start()
  {

    server.setMessageReceiveHandler([](std::string message){ std::cout << "Message reached server: " << message << std::endl; });

    server.helloHandler = [this](sockaddr_in address)
    {
      ClientInfo clientInfo;
      clientInfo.address = address;
      this->clients.push_back(clientInfo);
    };
    server.start();
  }

  void broadcast() {
    for (auto &client : clients) {
      
    }
  }
};

int main()
{

  Server server;
  server.start();

  return 0;
}