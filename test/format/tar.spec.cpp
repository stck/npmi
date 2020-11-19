#include "../../src/format/tar.hpp"
#include <cassert>
#include <tuple>

namespace tar {
  void test_decodeOctal() {
    auto map = {
        std::make_tuple(std::vector<char>{'0', '0', '0', '0', '0', '0', '0', '2', '1', '2', '2', 0}, 1106)};
    for (const auto& i : map) {
      auto [vec, result] = i;  // NOLINT(performance-unnecessary-copy-initialization)

      int dep = decodeOctal(vec.data(), vec.size());
      assert(dep == result);
    }
  }

  void test_decodeString() {
    auto map = {
        std::make_tuple(std::vector<char>{'h', 'e', 'l', 'l', 'o', 0, 0, 0, 0, 0}, "hello"),
        std::make_tuple(std::vector<char>{'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', 0}, "hello world")};
    for (const auto& i : map) {
      auto [vec, result] = i;  // NOLINT(performance-unnecessary-copy-initialization)

      std::string dep = decodeString(vec.data(), vec.size());
      assert(dep == result);
    }
  }
}  // namespace tar

auto main() -> int {
  tar::test_decodeOctal();
  tar::test_decodeString();

  return 0;
}
