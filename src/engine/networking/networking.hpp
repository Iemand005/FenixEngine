
#include "udp.cpp"
#include "packets.hpp"

typedef void (* MessageReceiveHandler)(std::string message);

class NetworkerClient {
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
};

class NetworkerServer {
  public:
  UDPServer server;
  MessageReceiveHandler messageReceiveHandler = nullptr;
  NetworkerServer() {
    server.init();
  }

  void start() {
    server.startListening([](const char* data, size_t size) {
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
          
          std::cout << "Received message: " << message << std::endl;
          std::string messageStr(message, messageLength);
          std::cout << "Received message: " << messageStr << std::endl;
          // if (messageReceiveHandler != nullptr) messageReceiveHandler(message);
        }
        break;
      }
    });
  }
};