#include "ifbstream.h"
#include <iostream>

auto ifbstream::next() -> unsigned int {
  if (pos >= 8) {
    pos = 0;
    if (!in.get(buf)) {
      throw std::ios_base::failure("Error reading compressed block");
    }
  }
  unsigned int bit = (buf >> pos) & 0x01;
  pos++;
  return bit;
}

auto ifbstream::read(int count) -> unsigned int {
  unsigned int bits = 0;
  unsigned int bit;

  for (int i = 0; i < count; i++) {
    bit = next();
    bits |= (bit << i);
  }
  return bits;
}
