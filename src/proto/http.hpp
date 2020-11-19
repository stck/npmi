#include <map>
#include <string>
#include <vector>

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
#  define INVALID_SOCKET -1
#  ifndef SOCK_NONBLOCK
#    define SOCK_NONBLOCK O_NONBLOCK
#  endif
#endif

#ifndef BUF_SIZE
#  define BUF_SIZE 102
#endif

#define LE "\r\n"
#define CONTENT_LENGTH "Content-Length"

namespace http {
  using Headers = std::map<std::string, std::string>;
  struct Response {
    Headers headers;
    std::vector<char> content;
    int size{};
  };

  class ConnectionException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

  class TransferException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

  namespace {
    using Socket           = unsigned long long;
    using ConnectionResult = int;
    using SocketAddress    = sockaddr_in;

    auto createSocket() -> Socket {
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
      int flags = fcntl(sock, F_GETFL, 0);
      ::fcntl(sock, F_SETFL, flags | SOCK_NONBLOCK);
#endif

      return sock;
    }

    void closeSocket(Socket _socket) {
#ifdef _WIN32
      ::closesocket(_socket);
#else
      close(_socket);
#endif
    }

    auto detectHost(const std::string& host, const short port) -> SocketAddress {
      SocketAddress sai{};
      struct hostent* _host = ::gethostbyname(host.c_str());

      if (_host) {
        sai.sin_port        = htons(port);
        sai.sin_family      = AF_INET;
        sai.sin_addr.s_addr = decltype(sai.sin_addr.s_addr)(*(reinterpret_cast<unsigned long*>(_host->h_addr)));
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

    auto send_request(Socket socket, const std::string& _method, const std::string& _uri, const std::string& _host, const Headers& _headers) -> int {
      std::string requestString;
      requestString.append(_method).append(" ").append(_uri).append(" ").append("HTTP/1.1").append(LE);
      requestString.append("Host:").append(" ").append(_host);
      for (auto const& [key, value] : _headers) {
        requestString.append(LE).append(key).append(": ").append(value);
      }
      requestString.append(LE).append(LE);
      return ::send(socket, requestString.c_str(), requestString.length(), 0);
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

    auto parse_headers(const std::string& headline) {
      Headers headers;

      int keyStart   = headline.find(CONTENT_LENGTH);
      int valueStart = headline.find_first_not_of(": ", headline.find_first_of(':', keyStart));
      int valueEnd   = headline.find_first_of(10, keyStart);
      headers.emplace(CONTENT_LENGTH, headline.substr(valueStart, valueEnd - valueStart - 1));

      return headers;
    }

    auto parse_response(const std::vector<char>& buffer) -> Response {
      std::vector<char> headline_buffer;
      std::vector<char> content_buffer;

      int i      = 3;
      bool found = false;
      for (; i < buffer.size(); i += 1) {
        if (buffer[i - 3] == 13 && buffer[i - 1] == 13 && buffer[i - 2] == 10 && buffer[i] == 10) {
          found = true;
          break;
        }
      }
      if (!found) {
        throw TransferException("Malformed response");
      }

      std::copy(&buffer[0], &buffer[i + 1], std::back_inserter(headline_buffer));
      std::string headline(headline_buffer.begin(), headline_buffer.end());
      Headers _headers   = parse_headers(headline);
      int content_length = std::atoi(_headers.at(CONTENT_LENGTH).c_str());  // NOLINT(cert-err34-c)

      std::copy(&buffer[i + 1], &buffer[i + 1 + content_length], std::back_inserter(content_buffer));

      return Response{
          .headers = _headers,
          .content = content_buffer,
          .size    = content_length};
    }
  }  // namespace


  class Client {
  private:
    Socket socket = INVALID_SOCKET;  // NOLINT(hicpp-signed-bitwise)

  protected:
    std::string& _host;
    short _port = 80;
    Headers _headers{
        {"Connection", "close"},
        {"Accept", "*/*"},
        {"User-Agent", "cpp-http/1.0"}};
    struct timeval timeout {
      .tv_sec  = 30,
      .tv_usec = 0
    };

    [[nodiscard]] auto isConnected() const noexcept -> bool {
      return this->socket != INVALID_SOCKET;
    }  // NOLINT(hicpp-signed-bitwise)

    void Close() const {
      closeSocket(this->socket);
    }

  public:
    explicit Client(std::string& host)
        : _host(host) {}

    auto Request(const std::string& method, const std::string& path) {
      Response response;
      try {
        if (!this->isConnected()) {
          this->socket = createSocket();
          auto sa      = detectHost(this->_host, this->_port);
          connect(this->socket, sa, &this->timeout);
        }

        auto bytesWrote = send_request(this->socket, method, path, this->_host, this->_headers);
        if (bytesWrote == 0) {
          throw TransferException{"Unable to transfer request to source"};
        }

        auto buffers = receive_response(this->socket, &this->timeout);
        response     = parse_response(buffers);

        this->Close();  //do not close if keepalived
      } catch (std::exception& e) {
        closeSocket(this->socket);
        throw e;
      }

      return response;
    }

    auto Download(const std::string& path) {
      return this->Request("GET", path);
    }
  };

}  // namespace http

#undef BUF_SIZE
#undef CONTENT_LENGTH
#undef LE
#ifndef _WIN32
#  undef INVALID_SOCKET
#endif
