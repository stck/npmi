#include "../../src/proto/tcp.hpp"
#include <iostream>

namespace tcp {
  static Socket sock;

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


  auto runAll() {
    test_createSocket();
  }
}  // namespace tcp


auto main() -> int {
  tcp::runAll();
  return 0;
};
