
#pragma once
#include <map>
#include <thread>

#include "packets.hpp"

#ifdef FE_WIN32
#include "udp.hpp"
#endif

typedef void (*MessageReceiveHandler)(std::string message);

struct ClientData {
  unsigned char id;
  sockaddr_in address;
  std::string username;
};

class Networker {
 public:
  using MessageReceiveHandler = std::function<void(std::string message, const ClientData sender)>;
  using ReceiveHandler = std::function<void(const char* data, size_t size, PacketType type, const ClientData sender)>;
  MessageReceiveHandler messageReceiveHandler;

  ReceiveHandler receiveHandler = nullptr;
  UDPSocket socket;

  std::thread listenerThread;

  // std::vector<ClientData> clients = std::vector<ClientData>();
  std::unordered_map<sockaddr_in, ClientData, sockaddr_in_hash, sockaddr_in_equal> clients = std::unordered_map<sockaddr_in, ClientData, sockaddr_in_hash, sockaddr_in_equal>(MAX_PLAYER_COUNT);
  std::unordered_map<unsigned char, ClientData> clientClients = std::unordered_map<unsigned char, ClientData>();

  unsigned short port = 0;
  std::string serverAddress = "127.0.0.1";

  unsigned char lastClientId = 0;

  std::string username = "Server";

  bool registeredOnServer = false;
  bool isServer = false;

  Networker() {}

  Networker(unsigned short port) { this->port = port; }

  void connect(std::string address, unsigned short port, std::string username) {
    this->port = port;
    this->serverAddress = address;
    this->username = username;
    this->clientClients = std::unordered_map<unsigned char, ClientData>();
    this->sendHello();
    this->startAsync(0);
  }

  template <typename T>
  void send() {
    T packet;
    this->send<T>(packet);
  }

  template <typename T>
  void send(T packet) {
    this->socket.send((char*)&packet, sizeof(T), this->port, this->serverAddress);
  }

  template <typename T>
  void send(T packet, const sockaddr_in& address) {
    this->socket.send((char*)&packet, sizeof(T), address);
  }

  void sendPing() { this->socket.send<PingPacket>(port); }

  size_t copyStr(char* target, std::string source) {
    size_t size = source.size();
    memcpy(target, source.c_str(), size);
    return size;
  }

  void sendHello() {
    HelloPacket packet;
    // memcpy(packet.username, username.c_str(), username.size());
    copyStr(packet.clientInfo.username, username);
    packet.clientInfo.usernameLength = username.size();
    this->socket.send<HelloPacket>(packet, this->port, this->serverAddress);
    // this->socket.send((char*)&packet, sizeof(HelloPacket), port);
  }

  void sendMessage(std::string message) {
    constexpr size_t headerSize = sizeof(MessagePacket);
    size_t totalSize = sizeof(MessagePacket) + message.size();
    char* packet = (char*)malloc(totalSize);
    MessagePacket messagePacket{};
    messagePacket.messageLength = message.size();

    memcpy(packet, &messagePacket, sizeof(MessagePacket));
    memcpy(packet + headerSize, message.c_str(), message.size());

    this->socket.send((char*)packet, totalSize, port);
  }

  void sendPosition(glm::vec3 position, glm::vec3 rotation) {
    PositionPacket packet;
    packet.position = position;
    packet.rotation = rotation;
    this->socket.send<PositionPacket>(packet, port);
  }

  void setMessageReceiveHandler(MessageReceiveHandler handler) { messageReceiveHandler = handler; }

  void broadcast(const char* data, size_t size, const sockaddr_in& from) {
    auto sender = clients.at(from);
    this->broadcast(data, size, sender);
  }

  void broadcast(const char* data, size_t size, ClientData sender) {
    std::cout << "Boradcasting" << std::endl;
    for (auto& [address, client] : clients) {
      if (client.id == sender.id && sockaddr_in_equal{}(address, sender.address)) {
        std::cout << "Not sending to source" << std::endl;
        continue;
        ;
      }
      auto packet = (PacketHeader*)data;
      packet->clientId = sender.id;
      this->socket.send(data, size, client.address);
    }
  }

  template <typename T>
  void broadcast(T packet) {
    std::cout << "Boradcasting" << std::endl;
    for (auto& [address, _] : clients) {
      this->send(packet, address);
    }
  }

  template <typename T>
  void broadcast(T packet, const sockaddr_in& from) {
    this->broadcast(packet, clients.at(from));
  }

  template <typename T>
  void broadcast(T packet, ClientData sender) {
    std::cout << "Broadcasting" << std::endl;
    for (auto& [address, _] : clients) {
      packet.header.clientId = sender.id;
      this->send(packet, address);
    }
  }

  void start() { this->start(this->port); }
  void startServer() {
    isServer = true;
    start();
  }

  template <typename T>
  T dataAs(const char* data) {
    return *(T*)data;
  }

  void start(unsigned short port) {
    socket.startListening(port, [this](const char* data, size_t size, const sockaddr_in& from) {
      if (size < sizeof(PacketHeader)) {
        std::cout << "Received a packet but it's too small";
        return;
      }

      auto header = this->dataAs<PacketHeader>(data);

      ClientData sender;
      if (isServer && clients.size()) {
        if (clients.count(from))
          // if(!clients.size()) return;
          sender = this->clients.at(from);
      } else {
        if (clientClients.count(header.clientId)) sender = this->clientClients.at(header.clientId);
      }

      std::cout << "Identified packet sender: (#" << (int)sender.id << ") Username: " << sender.username << std::endl;

      // maype make one handler before default and one after
      if (header.type != PacketType::Hello) this->broadcast(data, size, sender);

      // auto header = *(PacketHeader*)data;
      // PacketHeader headerS = *header;

      switch (header.type) {
        case PacketType::Hello: {
          auto hello = this->dataAs<HelloPacket>(data);
          // auto hello = *(HelloPacket*)data;
          if (ntohs(from.sin_port) == this->port) {
            std::cout << "Received Hello from same port, ignoring" << std::endl;
            return;
          }
          ClientData clientInfo{};
          clientInfo.address = from;
          unsigned char id = lastClientId++;
          clientInfo.id = id;
          clientInfo.username = std::string(hello.clientInfo.username, hello.clientInfo.usernameLength);
          this->clients.insert_or_assign(from, clientInfo);
          clientClients.insert_or_assign(clientInfo.id, clientInfo);
          std::cout << "Client added to connection list. Assigned ID: " << (int)id << " Their username is: " << clientInfo.username << std::endl;

          this->socket.send<HelloOkPacket>(from);

          ClientListPacket clientList;
          clientList.clientCount = clients.size();
          size_t i = 0;
          for (auto& [address, client] : clients) {
            auto size = copyStr(clientList.clients[i].username, client.username);
            clientList.clients[i].usernameLength = size;
            clientList.clients[i].id = client.id;
            i++;

            std::cout << "I have user: " << client.username << std::endl;
          }

          this->broadcast(clientList);
          // this->socket.send<ClientListPacket>(clientList, from);
        } break;
        case PacketType::HelloOk: {
          std::cout << "The server said we're okay!" << std::endl;
          registeredOnServer = true;
          // helloHandler(from);

        } break;
        case PacketType::HelloNotOk: {
          auto hello = dataAs<HelloNotOkPacket>(data);
          std::cerr << "The server said you couldn't join... error code: " << hello.reason << std::endl;

        } break;
        case PacketType::ClientList: {
          std::cout << "The server showed us who's online!" << std::endl;
          auto clientList = dataAs<ClientListPacket>(data);
          for (size_t i = 0; i < clientList.clientCount; i++) {
            ClientInfo clientInfo = clientList.clients[i];
            ClientData client;
            client.id = clientInfo.id;
            client.username = std::string(clientInfo.username, clientInfo.usernameLength);
            std::cout << "Client number " << i << " has username: " << client.username << " and ID: " << client.id << std::endl;
            clientClients.insert_or_assign(client.id, client);
          }
        } break;
        case PacketType::Message: {
          auto messagePacket = (MessagePacket*)data;
          const size_t messageLength = messagePacket->messageLength;
          char* messageBuffer = (char*)malloc(messageLength);
          memcpy(messageBuffer, data + sizeof(MessagePacket), messageLength);

          std::string message(messageBuffer, messageLength);
          if (isServer) {
            ClientData data = clients.at(from);
            data.id;
            if (messageReceiveHandler != nullptr) messageReceiveHandler(message, data);
          } else {
            ClientData data = clientClients.at(header.clientId);
            if (messageReceiveHandler != nullptr) messageReceiveHandler(message, data);
          }
        } break;
        case PacketType::Position: {
          auto packet = (PositionPacket*)data;
          packet->position;
          std::cout << "X: " << packet->position.x << " Y: " << packet->position.y << " Z: " << packet->position.z << std::endl;
          packet->rotation;
        } break;
        case PacketType::Ping: {
          std::cout << "Received a ping! Answering with pong..." << std::endl;
          this->socket.send<PongPacket>(from);
        } break;
        case PacketType::Pong: {
          std::cout << "Received a PONG!" << std::endl;
        } break;
      }

      if (receiveHandler) receiveHandler(data, size, header.type, sender);
    });
  }

  void stopAsync() {
    // if (this->listenerThread) this->listenerThread.();
  }

  void startAsync(unsigned short port) {
    stopAsync();
    listenerThread = std::thread([this, port]() { this->start(port); });
    listenerThread.detach();
  }
};