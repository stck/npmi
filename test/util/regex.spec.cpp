#include "../../src/util/regex.h"
#include <cassert>

namespace regex {

  void test_translate() {
    auto map = {
        std::make_tuple("license", "^.*license$"),
        std::make_tuple("license*", "^.*license.*$"),
        std::make_tuple("lice?se*", "^.*lice.?se.*$"),
        std::make_tuple("lice?se~", "^.*lice.?se\\~$")};

    for (const auto& a : map) {
      auto [source, result] = a;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = translate(source);
      assert(dep == result);
    }
  }

  void test_convert() {
    std::vector<std::string> map = {
        "license",
        "license*",
        "lice?se*"};
    auto wl = convert(map);

    assert(wl.size() == map.size());
  }

  void test_test() {
    std::string pattern                  = "lice?se*";
    auto translated                      = translate(pattern);
    std::vector<std::string> patternList = {pattern};
    auto regexList                       = convert(patternList);
    std::regex re("^.*lice.?se.*$", std::regex::icase);

    auto map = {
        std::make_tuple("./node_moduls/@some-org/lib/LICENSE.md", true),
        std::make_tuple("./node_moduls/@some-org/lib/LICENSE.MD", true),
        std::make_tuple("./node_moduls/@some-org/lib/LICENSE.markdown", true),
        std::make_tuple("./node_moduls/@some-org/lib/LICEgSE.markdown", true),
        std::make_tuple("./node_moduls/@some-org/lib/LICENNSE.markdown", false),
        std::make_tuple("./node_moduls/@some-org/lib/LICENSE", true),
        std::make_tuple("./node_moduls/@some-org/lib/README", false),
    };
    for (const auto& j : map) {
      const auto [_test, result] = j;
      assert(test(_test, regexList) == result);
    }
  }
}  // namespace regex


auto main() -> int {
  regex::test_translate();
  regex::test_convert();
  regex::test_test();

  return 0;
}
