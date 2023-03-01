#ifndef IFBSTREAM_H
#define IFBSTREAM_H

#include <bitset>
#include <fstream>
#include <string>

using byte = char;

class ifbstream {
  std::ifstream in;
  byte buf;
  size_t pos;

  static constexpr const std::ios_base::openmode ibmode = std::ios::in | std::ios::binary;

public:
  ifbstream(std::string fn)
      : in(fn, ibmode), pos(8) {
  }

  ifbstream(std::ifstream& in)
      : in(std::move(in)), pos(8) {
  }

  auto next() -> unsigned int;
  auto read(int count) -> unsigned int;

  inline void open(const char* fn) {
    in.open(fn, ibmode);
    pos = 8;
  }
  inline void close() {
    in.close();
  }
  inline void reset() {
    in.seekg(0);
    pos = 8;
  }
  inline auto tellg() -> std::streampos {
    return in.tellg();
  }
};

#endif
