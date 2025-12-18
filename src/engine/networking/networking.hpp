
#pragma once
#include <thread>

#include "udp.hpp"
#include "packets.hpp"

typedef void (*MessageReceiveHandler)(std::string message);

class Networker
{
public:
  using MessageReceiveHandler = std::function<void(std::string message)>;
  using AllPacketHandler = std::function<void(const char *data, size_t size, const sockaddr_in &from)>;
  MessageReceiveHandler messageReceiveHandler;

  AllPacketHandler allPacketHandler = nullptr;
  UDPSocket socket;

  std::thread listenerThread;

  std::vector<ClientInfo> clients = std::vector<ClientInfo>();

  unsigned short port = 0;

  unsigned int lastClientId = 0;

  Networker() {}

  Networker(unsigned short port)
  {
    this->port = port;
  }

  void connect()
  {
    this->sendHello();
    this->startAsync(0);
  }

  void sendPing()
  {
    this->socket.send<PingPacket>(port);
  }

  void sendHello()
  {
    HelloPacket packet;
    this->socket.send((char *)&packet, sizeof(HelloPacket), port);
  }

  void sendMessage(std::string message)
  {

    constexpr size_t headerSize = sizeof(MessagePacket);
    size_t totalSize = sizeof(MessagePacket) + message.size();
    char *packet = (char *)malloc(totalSize);
    MessagePacket messagePacket{};
    messagePacket.messageLength = message.size();

    memcpy(packet, &messagePacket, sizeof(MessagePacket));
    memcpy(packet + headerSize, message.c_str(), message.size());

    this->socket.send((char *)packet, totalSize, port);
  }

  void sendPosition(glm::vec3 position, glm::vec3 rotation)
  {
    PositionPacket packet;
    packet.position = position;
    packet.rotation = rotation;
    this->socket.send((char *)&packet, sizeof(PositionPacket), port);
  }

  void setMessageReceiveHandler(MessageReceiveHandler handler)
  {
    messageReceiveHandler = handler;
  }

  void start()
  {
    this->start(this->port);
  }

  void start(unsigned short port)
  {
    socket.startListening(port, [this](const char *data, size_t size, const sockaddr_in &from)
                          {
      if (size < sizeof(PacketHeader)) {
        std::cout << "Received a packet but it's too small";
        return;
      }

      if (allPacketHandler) allPacketHandler(data, size, from);

      auto header = (PacketHeader*)data;
      PacketHeader headerS = *header;

      switch (headerS.type) {
        case PacketType::Hello:
        {
          ClientInfo clientInfo;
      clientInfo.address = from;
      // clientInfo.id = *lastClientId++;
      this->clients.push_back(clientInfo);
      std::cout << "Client added to connection list." << std::endl;
        }
        break;
        case PacketType::Ok:
        {
          std::cout << "The server said we're okay!" << std::endl;
          // helloHandler(from);
        }
        break;
        case PacketType::Message:
        {
          auto messagePacket = (MessagePacket*)data;
          const size_t messageLength = messagePacket->messageLength;
          char* messageBuffer = (char*)malloc(messageLength);
          memcpy(messageBuffer, data + sizeof(MessagePacket), messageLength);
          
          std::string message(messageBuffer, messageLength);
          if (messageReceiveHandler != nullptr) messageReceiveHandler(message);
        }
        break;
        case PacketType::Position:
        {
          auto packet = (PositionPacket*)data;
          packet->position;
          std::cout << "X: " << packet->position.x << " Y: " << packet->position.y << " Z: " << packet->position.z << std::endl;
          packet->rotation;
        }
        break;
         case PacketType::Ping:
        {
          std::cout << "Received a ping! Answering with pong..." << std::endl;
          this->socket.send<PongPacket>(from);
        }
        break;
        case PacketType::Pong:
        {
          std::cout << "Received a PONG!" << std::endl;
        }
        break;
      } });
  }

  void startAsync(unsigned short port)
  {
    listenerThread = std::thread([this, port]()
                                 { this->start(port); });
    listenerThread.detach();
  }
};