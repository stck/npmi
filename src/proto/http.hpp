#include "tcp.hpp"
#include <map>
#include <string>
#include <vector>


#define CONTENT_LENGTH "Content-Length"

namespace http {
  using headers = std::map<std::string, std::string>;
  struct response {
    headers headers;
    std::vector<char> content;
    int size{};
  };


  namespace {


    auto parse_headers(const std::string& headline) {
      headers headers;

      size_t keyStart   = headline.find(CONTENT_LENGTH);
      size_t valueStart = headline.find_first_not_of(": ", headline.find_first_of(':', keyStart));
      size_t valueEnd   = headline.find_first_of(10, keyStart);
      headers.emplace(CONTENT_LENGTH, headline.substr(valueStart, valueEnd - valueStart - 1));

      return headers;
    }

    auto send_request(tcp::Socket socket, const std::string& _method, const std::string& _uri, const std::string& _host, const headers& _headers) -> int {
      std::string requestString;
      requestString.append(_method).append(" ").append(_uri).append(" ").append("HTTP/1.1").append(LE);
      requestString.append("Host:").append(" ").append(_host);
      for (auto const& [key, value] : _headers) {
        requestString.append(LE).append(key).append(": ").append(value);
      }
      requestString.append(LE).append(LE);
      return ::send(socket, requestString.c_str(), requestString.length(), 0);
    }

    auto parse_response(const std::vector<char>& buffer) -> response {
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
      headers _headers   = parse_headers(headline);
      int content_length = std::atoi(_headers.at(CONTENT_LENGTH).c_str());  // NOLINT(cert-err34-c)

      std::copy(&buffer[i + 1], &buffer[i + 1 + content_length], std::back_inserter(content_buffer));

      return response{
          .headers = _headers,
          .content = content_buffer,
          .size    = content_length};
    }
  }  // namespace


  class client {
  private:
    Socket socket = INVALID_SOCKET;  // NOLINT(hicpp-signed-bitwise)

  protected:
    std::string& _host;
    short _port = 80;
    headers _headers{
        {"Connection", "close"},
        {"Accept", "*/*"},
        {"User-Agent", "npmci/1.0"}};
    struct timeval timeout {
      .tv_sec  = 30,
      .tv_usec = 0
    };

    [[nodiscard]] auto connected() const noexcept -> bool {
      return this->socket != INVALID_SOCKET;
    }  // NOLINT(hicpp-signed-bitwise)

    void close() const {
      close_socket(this->socket);
    }

  public:
    explicit client(std::string& host)
        : _host(host) {}

    auto request(const std::string& method, const std::string& path) {
      response response;
      try {
        if (!this->connected()) {
          this->socket = tcp::create_socket();
          auto sa      = tcp::detect_host(this->_host, this->_port);
          tcp::connect(this->socket, sa, &this->timeout);
        }

        auto bytesWrote = tcp::send_request(this->socket, method, path, this->_host, this->_headers);
        if (bytesWrote == 0) {
          throw tcp::TransferException{"Unable to transfer request to source"};
        }

        auto buffers = tcp::receive_response(this->socket, &this->timeout);
        response     = parse_response(buffers);

        this->close();  // do not close if keepalived
      } catch (std::exception& e) {
        close_socket(this->socket);
        throw e;
      }

      return response;
    }

    auto download(const std::string& path) {
      return this->request("GET", path);
    }
  };

}  // namespace http

#undef BUF_SIZE
#undef CONTENT_LENGTH
#undef LE
#ifndef _WIN32
#  undef INVALID_SOCKET
#endif
