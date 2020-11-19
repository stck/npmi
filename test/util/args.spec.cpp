#include "../../src/util/args.hpp"
#include <cassert>
#include <vector>


namespace args {
  void test_get() {
    _args = {
        {"foo", true},
        {"bar", false}};

    std::vector<std::tuple<std::string, bool, bool>> values = {
        std::make_tuple("foo", false, true),
        std::make_tuple("bar", true, false),
        std::make_tuple("baz", true, true),
        std::make_tuple("baz", false, false)};

    for (auto& i : values) {
      const auto& [key, fallback, result] = i;

      auto dep = get(key, fallback);

      assert(dep == result);
    }
  }
}  // namespace args


auto main() -> int {
  args::test_get();
}
