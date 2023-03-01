#ifndef NPM_TCP_HPP
#define NPM_TCP_HPP

#include <array>
#include <string>
#include <vector>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32")
#else
#  include <arpa/inet.h>
#  include <fcntl.h>
#  include <netdb.h>
#  include <poll.h>
#  include <unistd.h>
#  define INVALID_SOCKET (-1)
#  ifndef SOCK_NONBLOCK
#    define SOCK_NONBLOCK O_NONBLOCK
#  endif
#endif

#ifndef TIMEOUT
#  define TIMEOUT 1000u
#endif

#ifndef BUF_SIZE
#  define BUF_SIZE size_t(64u)
#endif

namespace tcp {
  using socket_t      = int;  // uint64_t
  using socket_addr_t = sockaddr_in;
  using sbuffer_t     = std::array<unsigned char, BUF_SIZE>;
  using buffer_t      = std::vector<unsigned char>;

  namespace {
    class ConnectionException : public std::runtime_error {
    public:
      using std::runtime_error::runtime_error;
    };

    class TransferException : public std::runtime_error {
    public:
      using std::runtime_error::runtime_error;
    };

    template<typename T>
    inline auto handle_interruption(T fn) -> ssize_t {
      ssize_t res;
      while (true) {
        res = fn();

        if (res < 0 && errno == EINTR) {
          continue;
        } else {
          break;
        }
      }
      return res;
    }

    auto create_socket() -> socket_t {
#ifdef _WIN32
      // winsock must be initialized before use
      WSADATA wsaData;
      if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {  // NOLINT(hicpp-signed-bitwise)
        WSACleanup();
      }
#endif
      socket_t sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32  // Non-blocking flags
      auto on = 1UL;
      ::ioctlsocket(sock, FIONBIO, &on);  // NOLINT(hicpp-signed-bitwise)
#else
      int flags = ::fcntl(sock, F_GETFL, 0);
      ::fcntl(sock, F_SETFL, flags | SOCK_NONBLOCK | SO_REUSEADDR | SO_REUSEPORT);
#endif
      return sock;
    }

    void close_socket(const socket_t& sock) {
#ifdef _WIN32
      ::closesocket(sock);
#else
      close(sock);
#endif
    }

    auto detect_host(const std::string& host, const uint16_t& port) -> socket_addr_t {
      socket_addr_t   sai{};
      struct hostent* _host = ::gethostbyname(host.c_str());

      if (_host && _host->h_addrtype == AF_INET) {
        sai.sin_port        = htons(port);
        sai.sin_family      = AF_INET;
        sai.sin_addr.s_addr = *reinterpret_cast<uint32_t*>(_host->h_addr);
      } else {
        throw ConnectionException{"Unable to resolve host"};
      }
      return sai;
    }

    auto poll_socket(const socket_t& sock, const int16_t events = POLLIN | POLLOUT | POLLERR) -> void {
      int pollStatus;

      struct pollfd pfd {
        .fd     = sock,
        .events = events,
      };

      do {
        pollStatus = ::poll(&pfd, 1, TIMEOUT);
      } while (pollStatus == 0);

      if (pollStatus > 0) {
        return;
      } else {
        throw ConnectionException{"Host connection timed-out"};
      }
    }

    auto connect(const socket_t& sock, socket_addr_t address) -> void {
      int   address_size   = sizeof(address);
      auto* socket_address = reinterpret_cast<sockaddr*>(&address);

      ::connect(sock, socket_address, address_size);  // NOLINT(bugprone-unused-return-value)
      try {
        poll_socket(sock);
      } catch (std::exception& e) {
        throw ConnectionException{"Unable to connect to host"};
      }
    }


    auto receive_socket(const socket_t& sock, sbuffer_t* buf) -> ssize_t {
      return handle_interruption([&]() -> ssize_t {
        return ::recv(sock, buf, buf->size(), 0);
      });
    }

    auto send_socket(const socket_t& sock, const sbuffer_t* buf) -> ssize_t {
      return handle_interruption([&]() -> ssize_t {
        return ::send(sock, buf, buf->size(), 0);
      });
    }

    auto send_socket(const socket_t& sock, const sbuffer_t* buf, const ssize_t len) -> ssize_t {
      return handle_interruption([&]() -> ssize_t {
        return ::send(sock, buf, len, 0);
      });
    }
  }  // namespace

  auto socket_to_host(const std::string& host, const uint16_t& port) -> socket_t {
    auto sock = create_socket();
    auto addr = detect_host(host, port);

    connect(sock, addr);

    return sock;
  }

  [[maybe_unused]] auto is_socket_valid(const socket_t& sock) noexcept -> bool {
    if (sock == INVALID_SOCKET) return false;

    try {
      poll_socket(sock);

      return true;
    } catch (std::exception&) {
      return false;
    }
  }

  auto receive_bytes(const socket_t& sock) -> buffer_t {
    buffer_t buf;
    poll_socket(sock, POLLIN | POLLERR);

    while (true) {
      sbuffer_t buffer{};

      auto data_bytes = receive_socket(sock, &buffer);
      if (data_bytes > 0) {
        std::copy(&buffer[0], &buffer[data_bytes], std::back_inserter(buf));
      } else if (data_bytes == 0) {
        break;
      } else {
        if (errno == 0x36 || errno == 0x23) {  // EINPROGRESS if recv is waiting for next step
          break;
        }

        throw TransferException{"Unable to receive response bytes"};
      }
    }

    return buf;
  }

  auto send_bytes(const socket_t& sock, const buffer_t& buffer) -> void {
    poll_socket(sock, POLLOUT | POLLERR);
    ssize_t offset = 0;
    ssize_t rest;

    do {
      sbuffer_t partial_buffer{};
      ssize_t   data_bytes;
      std::copy_n(buffer.begin() + offset, partial_buffer.size(), partial_buffer.begin());
      if ((rest = buffer.size() - offset) < partial_buffer.size()) {
        data_bytes = send_socket(sock, &partial_buffer, rest);
      } else {
        data_bytes = send_socket(sock, &partial_buffer);
      }

      if (data_bytes > 0) {
        offset += data_bytes;
        continue;
      } else if (data_bytes == 0) {
        break;
      } else {
        throw TransferException{"Unable to write to socket"};
      }
    } while (offset <= buffer.size());
  }
}  // namespace tcp
#endif  // NPM_TCP_HPP