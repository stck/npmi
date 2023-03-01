#ifndef NPM_HTTP_HPP
#define NPM_HTTP_HPP

#include "tcp.hpp"
#include <format>
#include <map>
#include <string>
#include <vector>

#define CONTENT_LENGTH "Content-Length"
#define LE "\r\n"
#define SC " "
#define MC ":"
#define HOST "Host: "
#define HTTP "HTTP/1.1"

namespace http {
  using headers = std::map<std::string, std::string>;
  struct response {
    headers           headers;
    std::vector<char> content;
    int               size{};
  };


  namespace {
    class HTTPException : public std::runtime_error {
    public:
      using std::runtime_error::runtime_error;
    };

    auto parse_headers(const std::string& headline) {
      headers headers;

      size_t keyStart   = headline.find(CONTENT_LENGTH);
      size_t valueStart = headline.find_first_not_of(": ", headline.find_first_of(MC, keyStart));
      size_t valueEnd   = headline.find_first_of(10, keyStart);
      headers.emplace(CONTENT_LENGTH, headline.substr(valueStart, valueEnd - valueStart - 1));

      return headers;
    }

    auto construct_body(const std::string& method, const std::string& uri, const std::string& host, const headers& headers) -> std::string {
      std::string requestString;
      requestString.append(method).append(SC).append(uri).append(SC).append(HTTP).append(LE);
      requestString.append(HOST).append(host).append(LE);
      for (auto const& [key, value] : headers) {
        requestString.append(key).append(MC).append(SC).append(value).append(LE);
      }
      requestString.append(LE);

      return requestString;
    }

    auto parse_response(const tcp::buffer_t& buffer) -> response {
      std::vector<char> headline_buffer;
      std::vector<char> content_buffer;

      int i = 3;
      for (; i < buffer.size(); i += 1) {
        if (buffer[i - 3] == 13 && buffer[i - 1] == 13 && buffer[i - 2] == 10 && buffer[i] == 10) {
          break;
        }
      }
      if (i == buffer.size()) {
        throw tcp::TransferException("Malformed response");
      }

      std::copy(&buffer[0], &buffer[i + 1], std::back_inserter(headline_buffer));
      std::string headline(headline_buffer.begin(), headline_buffer.end());
      headers     _headers       = parse_headers(headline);
      int         content_length = std::atoi(_headers.at(CONTENT_LENGTH).c_str());  // NOLINT(cert-err34-c)

      std::copy(&buffer[i + 1], &buffer[i + 1 + content_length], std::back_inserter(content_buffer));

      return response{
          .headers = _headers,
          .content = content_buffer,
          .size    = content_length};
    }
  }  // namespace

  auto download(const std::string& uri, const int16_t port) {
    const std::string host_path = uri.substr(uri.find("://") + 3);
    const std::string host      = host_path.substr(0, host_path.find_first_of('/'));
    const std::string path      = host_path.substr(host_path.find_first_of('/'));
    headers           _headers{
        {"Connection", "close"},
        {"Accept", "*/*"},
        {"User-Agent", "npmci/1.0"}};

    tcp::socket_t sock;
    try {
      sock             = tcp::socket_to_host(host, port);
      std::string body = construct_body("GET", path, host, _headers);

      tcp::buffer_t buffer(body.c_str(), body.c_str() + body.size());
      buffer.push_back('\0');
      tcp::send_bytes(sock, buffer);

      // get response
      tcp::buffer_t buf = tcp::receive_bytes(sock);
      //std::string   noop{buf.begin(), buf.end()};

      tcp::close_socket(sock);

      return parse_response(buf);
    } catch (std::exception& e) {
      tcp::close_socket(sock);
      throw HTTPException{"Could not download file at " + uri};
    }
  }

  auto download(const std::string& uri) {
    return download(uri, 80);
  }

  //  class client {
  //  private:
  //    Socket socket = INVALID_SOCKET;  // NOLINT(hicpp-signed-bitwise)
  //
  //  protected:
  //    std::string& _host;
  //    short        _port = 80;
  //    headers      _headers{
  //             {"Connection", "close"},
  //             {"Accept", "*/*"},
  //             {"User-Agent", "npmci/1.0"}};
  //    struct timeval timeout {
  //      .tv_sec  = 30,
  //      .tv_usec = 0
  //    };
  //
  //    [[nodiscard]] auto connected() const noexcept -> bool {
  //      return this->socket != INVALID_SOCKET;
  //    }  // NOLINT(hicpp-signed-bitwise)
  //
  //    void close() const {
  //      close_socket(this->socket);
  //    }
  //
  //  public:
  //    explicit client(std::string& host)
  //        : _host(host) {}
  //
  //    auto request(const std::string& method, const std::string& path) {
  //      response response;
  //      try {
  //        if (!this->connected()) {
  //          this->socket = tcp::create_socket();
  //          auto sa      = tcp::detect_host(this->_host, this->_port);
  //          tcp::connect(this->socket, sa, &this->timeout);
  //        }
  //
  //        auto bytesWrote = tcp::send_request(this->socket, method, path, this->_host, this->_headers);
  //        if (bytesWrote == 0) {
  //          throw tcp::TransferException{"Unable to transfer request to source"};
  //        }
  //
  //        auto buffers = tcp::receive_bytes(this->socket, &this->timeout);
  //        response     = parse_response(buffers);
  //
  //        this->close();  // do not close if keepalived
  //      } catch (std::exception& e) {
  //        close_socket(this->socket);
  //        throw e;
  //      }
  //
  //      return response;
  //    }
  //
  //    auto download(const std::string& path) {
  //      return this->request("GET", path);
  //    }
  //  };

}  // namespace http

#undef BUF_SIZE
#undef CONTENT_LENGTH
#undef LE
#ifndef _WIN32
#  undef INVALID_SOCKET
#endif
#endif  // NPM_HTTP_HPP