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

int main()
{

  auto clients = std::vector<std::unique_ptr<ClientInfo>>();

  NetworkerServer server;

  server.setMessageReceiveHandler([](std::string message)
                                  { std::cout << "Message reached server: " << message << std::endl; });

                            server.helloHandler = [clients] (sockaddr_in address) {
                              auto clientInfo = std::make_unique<ClientInfo>();
                              clientInfo->address = address;
                              clients.push_back(clientInfo);
                            };
  server.start();

  return 0;
}