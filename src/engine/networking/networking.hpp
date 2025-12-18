
#pragma once
#include <thread>

#include "udp.hpp"
#include "packets.hpp"

typedef void (* MessageReceiveHandler)(std::string message);

class Networker {
  public:
  using MessageReceiveHandler = std::function<void(std::string message)>;
  using AllPacketHandler = std::function<void(const char* data, size_t size, const sockaddr_in& from)>;
  MessageReceiveHandler messageReceiveHandler;
  using HelloHandler = std::function<void(sockaddr_in address)>;
  HelloHandler helloHandler;

  AllPacketHandler allPacketHandler = nullptr;
  UDPSocket socket;

  unsigned short port = 0;

  Networker() {}

  Networker(unsigned short port) {
    this->port = port;
  }

  void connect(unsigned short port) {
    this->sendHello();
    this->start(0);
  }

  void sendPing() {
    this->socket.send<PingPacket>(port);
  }

  void sendHello() {
    PingPacket packet;
    this->socket.send((char*)&packet, sizeof(PingPacket), port);
  }

  void sendMessage(std::string message) {

    constexpr size_t headerSize = sizeof(MessagePacket);
    size_t totalSize = sizeof(MessagePacket) + message.size();
    char* packet = (char*)malloc(totalSize);
    MessagePacket messagePacket {};
    messagePacket.messageLength = message.size();

    memcpy(packet, &messagePacket, sizeof(MessagePacket));
    memcpy(packet + headerSize, message.c_str(), message.size());

    this->socket.send((char*)packet, totalSize, port);
  }

  void sendPosition(glm::vec3 position, glm::vec3 rotation) {
    PositionPacket packet;
    packet.position = position;
    packet.rotation = rotation;
    this->socket.send((char*)&packet, sizeof(PositionPacket), port);
  }
  
  void setMessageReceiveHandler(MessageReceiveHandler handler) {
    messageReceiveHandler = handler;
  }

  void start() {
    this->start(this->port);
  }

  void start(unsigned short port) {
    socket.startListening(port, [this](const char* data, size_t size, const sockaddr_in& from) {
      if (size < sizeof(PacketHeader)) {
        std::cout << "Received a packet but it's too small";
        return;
      }

      if (allPacketHandler) allPacketHandler(data, size, from);

      auto header = (PacketHeader*)data;

      switch (header->type) {
        case PacketType::Hello:
       {
         helloHandler(from);
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
          std::cout << "Received a ping!" << std::endl;
        }
        break;
      }
    });
  }

  void startAsync(unsigned short port) {
    std::thread listenerThread([this, port](){
      this->start(port);
    });
  }
};