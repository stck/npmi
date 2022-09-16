#include <string>

#ifdef _WIN32
#  include <winsock2.h>
#  pragma comment(lib, "ws2_32")
#else
#  include <arpa/inet.h>
#  include <fcntl.h>
#  include <netdb.h>
#  include <sys/ioctl.h>
#  include <sys/time.h>
#  include <unistd.h>
#  include <vector>
#  define INVALID_SOCKET (-1)
#  ifndef SOCK_NONBLOCK
#    define SOCK_NONBLOCK O_NONBLOCK
#  endif
#endif

#ifndef BUF_SIZE
#  define BUF_SIZE 102
#endif
#define LE "\r\n"

namespace tcp {
  using Socket           = uint64_t;
  using ConnectionResult = int;
  using SocketAddress    = sockaddr_in;

  class ConnectionException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

  class TransferException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

  namespace {
    auto create_socket() -> Socket {
#ifdef _WIN32
      // winsock must be initialized before use
      WSADATA wsaData;
      if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {  // NOLINT(hicpp-signed-bitwise)
        WSACleanup();
      }
#endif
      Socket sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32  // Non-blocking flags
      auto on = 1UL;
      ::ioctlsocket(sock, FIONBIO, &on);  // NOLINT(hicpp-signed-bitwise)
#else
      int flags = ::fcntl(sock, F_GETFL, 0);
      ::fcntl(sock, F_SETFL, flags | SOCK_NONBLOCK);
#endif
      return sock;
    }

    void close_socket(Socket _socket) {
#ifdef _WIN32
      ::closesocket(_socket);
#else
      close(_socket);
#endif
    }

    auto detect_host(const std::string& host, const u_int16_t port) -> SocketAddress {
      SocketAddress sai{};
      struct hostent* _host = ::gethostbyname(host.c_str());

      if (_host) {
        sai.sin_port        = htons(port);
        sai.sin_family      = AF_INET;
        sai.sin_addr.s_addr = static_cast<decltype(sai.sin_addr.s_addr)>(*(reinterpret_cast<unsigned long*>(_host->h_addr)));
      } else {
        throw ConnectionException{"Unable to resolve host"};
      }
      return sai;
    }

    void connect(Socket socket, SocketAddress address, timeval* to) {
      int address_size     = sizeof(address);
      auto* socket_address = reinterpret_cast<sockaddr*>(&address);
      fd_set fdSet;

      ::connect(socket, socket_address, address_size);  // NOLINT(bugprone-unused-return-value)

      FD_ZERO(&fdSet);
      FD_SET(socket, &fdSet);

      ConnectionResult cr = ::select(socket + 1UL, nullptr, &fdSet, nullptr, to);
      if (cr < 1) {
        throw ConnectionException{"Unable to connect to host"};
      }

      int so_error;
#ifdef _WIN32
      using socklen_t = int;
      using SockOpt_t = char*;
#else
      using SockOpt_t = void*;
#endif
      socklen_t len = sizeof so_error;
      ::getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<SockOpt_t>(&so_error), &len);

      if (so_error != 0) {
        throw ConnectionException("Timeout while acquiring connection to host");
      }
    }


    auto receive_bytes(Socket socket, char* buf, int len, timeval* to) -> int {
      fd_set fdSet;
      ConnectionResult connectionResult;

      FD_ZERO(&fdSet);
      FD_SET(socket, &fdSet);

      connectionResult = ::select(socket + 1UL, &fdSet, nullptr, nullptr, to);
      if (connectionResult == INVALID_SOCKET) return -2;  // timeout! NOLINT(hicpp-signed-bitwise)
#ifdef _WIN32
      if (connectionResult == SOCKET_ERROR) return -1;  // error
#else
      if (connectionResult == SO_ERROR) return -1;  // error
#endif
      return ::recv(socket, buf, len, 0);
    }

    auto receive_response(Socket socket, timeval* to) {
      std::vector<char> buf;

      while (true) {
        char buffer[BUF_SIZE]{};
        auto receivedBytes = receive_bytes(socket, buffer, BUF_SIZE, to);
        if (receivedBytes > 0) {
          std::copy(&buffer[0], &buffer[receivedBytes], std::back_inserter(buf));
        } else if (receivedBytes == 0) {
          break;
        } else {
          throw TransferException{"Unable to receive response bytes"};
        }
      }

      return buf;
    }
  }  // namespace


}  // namespace tcp