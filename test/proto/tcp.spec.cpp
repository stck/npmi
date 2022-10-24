#include "../../src/proto/tcp.hpp"
#include <iostream>

namespace tcp {
  static socket_t sock;

  auto test_createSocket() {
    std::vector<int> options{
        FWRITE,
        O_NONBLOCK,
    };

    sock = create_socket();

    assert(sock > 0);
#ifndef _WIN32
    int flags = fcntl(sock, F_GETFL, 0);

    for (const int flag : options) {
      assert(flags & flag);
    }
#endif

    close(sock);
  }

  auto test_closeSocket() {
#ifndef _WIN32
    assert((fcntl(sock, F_GETFD) != -1 || errno != EBADF) == 0);
#endif
    sock = create_socket();
#ifndef _WIN32
    assert((fcntl(sock, F_GETFD) != -1 || errno != EBADF) == 1);
#endif
    close_socket(sock);
#ifndef _WIN32
    assert((fcntl(sock, F_GETFD) != -1 || errno != EBADF) == 0);
#endif
  }

  auto test_detectHost() {
    auto host = detect_host("localhost", 80);

    assert(strcmp(inet_ntoa(host.sin_addr), "127.0.0.1") == 0);
    assert(host.sin_family == AF_INET);

    bool exceptionRaised = false;
    try {
      detect_host("888.265.0.0", 80);
    } catch (std::exception& e) {
      exceptionRaised = true;
    }

    assert(exceptionRaised);
  }

  auto test_connect() {
    sock      = create_socket();
    auto host = detect_host("localhost", 80);

    connect(sock, host);
  }


  auto runAll() {
    std::cout << "createSocket" << std::endl;
    test_createSocket();
    std::cout << "closeSocket" << std::endl;
    test_closeSocket();
    std::cout << "detectHost" << std::endl;
    test_detectHost();
    std::cout << "connect" << std::endl;
    test_connect();
    std::cout << "Finished" << std::endl;
  }
}  // namespace tcp


auto main() -> int {
  tcp::runAll();
  return 0;
};
