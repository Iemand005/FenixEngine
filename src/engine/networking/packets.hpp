#pragma once
#include <glm/glm.hpp>

#define MAX_PLAYER_COUNT 4

enum class PacketType : char {
  Invalid = 0,
  Hello,
  HelloOk,
  Ping,
  Pong,
  Position,
  Message,
  ClientList,
  ServerStatus,
};

struct PacketHeader {
  PacketType type;
  unsigned char version = 1;
  unsigned char clientId;
  unsigned int index;
};

struct ClientInfo {
  unsigned char id;
  unsigned char usernameLength = 32;
  char username[32];
};

struct ServerStatusPacket {
  PacketHeader header{PacketType::ServerStatus};

};

struct ClientListPacket {
  PacketHeader header{PacketType::ClientList};
  unsigned short clientCount;
  ClientInfo clients[MAX_PLAYER_COUNT];
};

struct HelloPacket {
  PacketHeader header{PacketType::Hello};
  ClientInfo clientInfo;
};

struct HelloOkPacket {
  PacketHeader header{PacketType::HelloOk};
};

struct PongPacket {
  PacketHeader header{PacketType::Pong};
};

struct PingPacket {
  PacketHeader header{PacketType::Ping};
};

struct MessagePacket {
  PacketHeader header{PacketType::Message};
  unsigned short messageLength;
  char message[0];
};

struct PositionPacket {
  PacketHeader header{PacketType::Message};
  glm::vec3 position;
  glm::vec3 rotation;
};
