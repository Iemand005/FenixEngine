
#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <system_error>
#include <functional>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
// typedef sockaddr_in=sockaddr_in
#define CLOSE_SOCKET closesocket
#define SOCKET_ERRNO WSAGetLastError()
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERRNO "idk"
    // #define CLOSE_SOCKET closesocket
        #define CLOSE_SOCKET close
#endif


struct sockaddr_in_hash {
    size_t operator()(const sockaddr_in& addr) const {
        // Combine IP and port into a single hash
        size_t h1 = std::hash<uint32_t>{}(addr.sin_addr.s_addr);
        size_t h2 = std::hash<uint16_t>{}(addr.sin_port);
        return h1 ^ (h2 << 1);
    }
};

struct sockaddr_in_equal {
    bool operator()(const sockaddr_in& a, const sockaddr_in& b) const {
        return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
    }
};

class UDPSocket
{
  SOCKET sock = INVALID_SOCKET;
  using ReceiveCallback = std::function<void(const char *data, size_t size, const sockaddr_in &from)>;

public:
  UDPSocket() {}

  bool makeAddress(unsigned short port, std::string address, sockaddr_in* socketAddress) {
    // socketAddress = new sockaddr_in();
    // socketAddress->sin_family= AF_INET;
    // socketAddress->sin_port = htons(port);
    
    sockaddr_in receiverAddr{};
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, address.c_str(), &socketAddress->sin_addr) <= 0)
    {
      std::cerr << "Invalid address\n";
      return false;
    }

    *socketAddress = receiverAddr;
    return true;
  }

  template <typename T>
  void send(unsigned short port, std::string address = "127.0.0.1") {
    T packet;
    this->send<T>(packet, port, address);
  }

  template <typename T>
  void send(T packet, unsigned short port, std::string address = "127.0.0.1") {
    this->send((char*)&packet, sizeof(T), port, address);
  }



  void send(const char *packet, size_t size, unsigned short port, std::string address = "127.0.0.1") {
    sockaddr_in receiverAddr{};
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, address.c_str(), &receiverAddr.sin_addr) <= 0)
    {
      std::cerr << "Invalid address\n";
      this->Close();
      return;
    }
    sockaddr_in receiverAddress;
    this->makeAddress(port, address, &receiverAddress);
    this->send(packet, size, receiverAddr);
  }



  template <typename T>
  void send(sockaddr_in address) {
    T packet;
    this->send<T>(packet, address);
  }

  template <typename T>
  void send(T packet, sockaddr_in address) {
    this->send((char*)&packet, sizeof(T), address);
  }

  void send(const char *packet, size_t size, sockaddr_in address)
  {
    this->createSocketIfNotExist();
    
    int sent = sendto(sock, packet, size, 0, (sockaddr *)&address, sizeof(address));

    if (sent >= 0)
    {
      char senderIP[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &address.sin_addr, senderIP, sizeof(senderIP));

      std::cout << "Sent " << sent << " bytes: " << (char *)packet <<" To " << senderIP << ":" << ntohs(address.sin_port) <<"\n";
    }
    else
    {
      std::cerr << "Send failed: " << SOCKET_ERRNO << "\n";
    }
  }

  bool createSocketIfNotExist() {
    if (sock != INVALID_SOCKET) return false;

    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
      std::cerr << "WSAStartup failed\n";
      return false;
    }
    #endif

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
      std::cerr << "Socket creation failed: " << SOCKET_ERRNO << "\n";
      return false;
    }


    this->sock = sock;

    std::cout << "Created new socket..." << std::endl;

    return true;
  }

  bool bindSocket() {
    bindSocket(0);
  }

  bool bindSocket(unsigned short port) {
    this->createSocketIfNotExist();

    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    localAddr.sin_port = htons(port);

    if (bind(sock, (sockaddr *)&localAddr, sizeof(localAddr)) < 0)
    {
      std::cerr << "Bind failed: " << SOCKET_ERRNO << "\n";
      CLOSE_SOCKET(sock);
      return false;
    }
    std::cout << "Bound socket..." << std::endl;
    return true;
  }

  int startListening(unsigned short port, ReceiveCallback callback)
  {
    // this->createSocketIfNotExist();
    if (port) this->bindSocket(port);
    std::cout << "Listening on UDP port " << port << "..." << std::endl;

    // Blocking mode
    // u_long mode = 1;
    // ioctlsocket(sock, FIONBIO, &mode);

    char buffer[1024];
    sockaddr_in senderAddr{};
    socklen_t senderLen = sizeof(senderAddr);

    while (true)
    {
      std::cout << "Waiting for next packet..." << std::endl;

      int received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&senderAddr, &senderLen);
     
      if (received >= 0)
      {
        buffer[received] = '\0';
        char senderIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &senderAddr.sin_addr, senderIP, sizeof(senderIP));

        std::cout << "Received " << received << " bytes from " << senderIP << ":" << ntohs(senderAddr.sin_port) << " - " << buffer << std::endl;

        callback(buffer, received, senderAddr);
      }
      else
      {
        std::cerr << "Receive failed: " << SOCKET_ERRNO << "\n";
        this->Close();
        // this->createSocketIfNotExist();
        this->bindSocket(port);
      }
    }

    this->Close();

    return 0;
  }

    void Close() {
    std::cerr << "Closing socket...\n";
    CLOSE_SOCKET(sock);
    sock = INVALID_SOCKET;

    #ifdef _WIN32
    WSACleanup();
    #endif
  }
};