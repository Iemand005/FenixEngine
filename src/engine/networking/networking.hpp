
#pragma once
#include <thread>

#include "udp.hpp"
#include "packets.hpp"

typedef void (* MessageReceiveHandler)(std::string message);

class NetworkerClient {
  public:
  UDPSocket socket;

  NetworkerClient() {

  }

  void connect() {
    this->sendHello();
  }

  void sendPing() {
    this->socket.send<PingPacket>();
  }

  void sendHello() {
    PingPacket packet;
    this->socket.send((char*)&packet, sizeof(PingPacket));
  }

  void sendMessage(std::string message) {

    constexpr size_t headerSize = sizeof(MessagePacket);
    size_t totalSize = sizeof(MessagePacket) + message.size();
    char* packet = (char*)malloc(totalSize);
    MessagePacket messagePacket {};
    messagePacket.messageLength = message.size();

    memcpy(packet, &messagePacket, sizeof(MessagePacket));
    memcpy(packet + headerSize, message.c_str(), message.size());

    this->socket.send((char*)packet, totalSize);
  }

  void sendPosition(glm::vec3 position, glm::vec3 rotation) {
    PositionPacket packet;
    packet.position = position;
    packet.rotation = rotation;
    this->socket.send((char*)&packet, sizeof(PositionPacket));
  }
};

class NetworkerServer {
  public:
  UDPSocket server;
  using MessageReceiveHandler = std::function<void(std::string message)>;
  using AllPacketHandler = std::function<void(const char* data, size_t size, const sockaddr_in& from)>;
  MessageReceiveHandler messageReceiveHandler;
  using HelloHandler = std::function<void(sockaddr_in address)>;
  HelloHandler helloHandler;

  AllPacketHandler allPacketHandler = nullptr;

  NetworkerServer() {
  }

  void setMessageReceiveHandler(MessageReceiveHandler handler) {
    messageReceiveHandler = handler;
  }

  void start() {
    server.startListening([this](const char* data, size_t size, const sockaddr_in& from) {
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

  void startAsync() {
    std::thread listenerThread([this](){
      this->start();
    });
  }
};