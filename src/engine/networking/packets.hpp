#pragma once
#include <glm/glm.hpp>

enum class PacketType : char {
  Invalid = 0,
  Message,
  Position,
};

struct PacketHeader {
  PacketType type;
  char version = 1;
  int index;
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