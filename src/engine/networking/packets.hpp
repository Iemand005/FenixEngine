#pragma once
#include <glm/glm.hpp>

enum class PacketType : char {
  Invalid = 0,
  Hello,
  HelloOk,
  Ping,
  Pong,
  Position,
  Message,
  ClientList,
};

struct PacketHeader {
  PacketType type;
  char version = 1;
  int index = 0;
  
};

struct ClientInfo {
char usernameLength = 32;
  char username[32];
};

struct ClientListPacket {
  PacketHeader header{PacketType::ClientList};
  short clientCount;
  ClientInfo clients[4];
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
  short messageLength;
  char message[0];
};

struct PositionPacket {
  PacketHeader header{PacketType::Message};
  glm::vec3 position;
  glm::vec3 rotation;
};
