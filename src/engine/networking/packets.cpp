

enum class PacketType : char {
  Invalid = 0,
  Message,
};

struct PacketHeader {
  PacketType type;
  char version = 1;
};

struct MessagePacket {
  PacketHeader header{PacketType::Message};
  char messageLength;
  char message[1];
};