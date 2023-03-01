#ifndef NPM_HTTPS_HPP
#define NPM_HTTPS_HPP

#include "http.hpp"
#include "tls.hpp"
#include <iostream>

namespace https {
  auto download(const std::string& uri, const int16_t port) {
    const std::string host_path = uri.substr(uri.find("://") + 3);
    const std::string host      = host_path.substr(0, host_path.find_first_of('/'));
    const std::string path      = host_path.substr(host_path.find_first_of('/'));
    http::headers     _headers{
        {"Connection", "close"},
        {"Accept", "*/*"},
        {"User-Agent", "npmci/1.0"}};

    tcp::socket_t sock;
    try {
      sock = tcp::socket_to_host(host, port);
      tls::handshake(sock);

      tcp::close_socket(sock);
    } catch (std::exception& e) {
      tcp::close_socket(sock);
      std::cout << e.what() << std::endl;
    }
  }

  auto download(const std::string& uri) {
    return download(uri, 443);
  }
}  // namespace https

#endif  // NPM_HTTPS_HPP