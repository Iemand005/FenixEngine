
#include "udp.cpp"
#include "packets.hpp"

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
  NetworkerServer() {}

  void start() {
    server.startListening([](const char* data, size_t size) {
      if (size < sizeof(PacketHeader)) {
        std::cout << "Received a packet but it's too small";
      }

      PacketHeader header;
      memcpy(&header, data, sizeof(PacketHeader));

      switch (header.type) {
        case PacketType::Message:
        {
          MessagePacket messagePacket;
          memcpy(&messagePacket, data, sizeof(MessagePacket));
        }
        break;
      }
    });
  }
};