
#pragma once
#include "udp.hpp"
#include "packets.hpp"

typedef void (* MessageReceiveHandler)(std::string message);

class NetworkerClient {
  public:
  UDPClient client;

  NetworkerClient() {

  }

  void connect() {
    this->client.openSocket();
  }

  void sendMessage(std::string message) {

    constexpr size_t headerSize = sizeof(MessagePacket);
    size_t totalSize = sizeof(MessagePacket) + message.size();
    char* packet = (char*)malloc(totalSize);
    MessagePacket messagePacket {};
    messagePacket.messageLength = message.size();

    memcpy(packet, &messagePacket, sizeof(MessagePacket));
    memcpy(packet + headerSize, message.c_str(), message.size());

    this->client.send((char*)packet, totalSize);
  }

  void sendPosition(glm::vec3 position, glm::vec3 rotation) {
    PositionPacket packet;
    packet.position = position;
    packet.rotation = rotation;
    this->client.send((char*)&packet, sizeof(PositionPacket));
  }
};

class NetworkerServer {
  public:
  UDPServer server;
  using MessageReceiveHandler = std::function<void(std::string message)>;
  MessageReceiveHandler messageReceiveHandler;

  NetworkerServer() {
    server.init();
  }

  void setMessageReceiveHandler(MessageReceiveHandler handler) {
    messageReceiveHandler = handler;
  }

  void start() {
    server.startListening([this](const char* data, size_t size, const sockaddr_in& from) {
      if (size < sizeof(PacketHeader)) {
        std::cout << "Received a packet but it's too small";
      }

      auto header = (PacketHeader*)data;
      // memcpy(&header, data, sizeof(PacketHeader));

      switch (header->type) {
        case PacketType::Message:
        {
          auto messagePacket = (MessagePacket*)data;
          // memcpy(&messagePacket, data, sizeof(MessagePacket));
          const size_t messageLength = messagePacket->messageLength;
          char* message = (char*)malloc(messageLength);
          memcpy(message, data + sizeof(MessagePacket), messageLength);
          
          // std::cout << "Received message: " << message << std::endl;
          std::string messageStr(message, messageLength);
          std::cout << "Received message: " << messageStr << std::endl;
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
      }
    });
  }
};