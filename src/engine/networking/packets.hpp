#pragma once
#include <glm/glm.hpp>

enum class PacketType : char {
  Invalid = 0,
  Hello,
  Ok,
  Identity,
  Identify,
  Ping,
  Pong,
  Position,
  Message,
};

// template<typename T>
// struct PacketBase<T> {
//   PacketHeader header{T};
// }

struct PacketHeader {
  PacketType type;
  char version = 1;
  int index = 0;
};

struct HelloPacket {
  PacketHeader header{PacketType::Hello};
};

struct IdentifyPacket {
  PacketHeader header{PacketType::Identify};
}

struct IdentityPacket {
  PacketHeader header{PacketType::Identity};
  char usernameLength = 32;
  char username[32];
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