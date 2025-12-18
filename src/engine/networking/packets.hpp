#pragma once
#include <glm/glm.hpp>

enum class PacketType : char {
  Invalid = 0,
  Hello,
  Ok,
  Ping,
  Pong,
  Position,
  Message,
};

struct PacketHeader {
  PacketType type;
  char version = 1;
  int index = 0;
};

struct HelloPacket {
  PacketHeader header{PacketType::Hello};
};

struct OkPacket {
  PacketHeader header{PacketType::Ok};
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