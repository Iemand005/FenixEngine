
#pragma once
#include "udp.hpp"
#include "packets.hpp"

typedef void (* MessageReceiveHandler)(std::string message);

class NetworkerClient {
  public:
  UDPSocket socket;

  NetworkerClient() {

  }

  void sendHello() {
    HelloPacket packet;
    this->socket.send((char*)&packet, sizeof(HelloPacket));
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
  MessageReceiveHandler messageReceiveHandler;
  using HelloHandler = std::function<void(sockaddr_in address)>;
  HelloHandler helloHandler;

  NetworkerServer() {
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
      }
    });
  }
};