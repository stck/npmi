#include "../../src/format/package_lock.hpp"
#include <cassert>
#include <tuple>


namespace packagelock {

  void test_V1Parser_Counts() {
    std::vector<std::pair<std::string, int>> map = {
        std::pair(R"({"name": "empty_lock"})", 0),
        std::pair(R"({"name": "empty_dep", "dependencies": {}})", 0),
        std::pair(R"({"name": "empty_dep", "dependencies": {"test": {}}})", 1),
        std::pair(R"({"name": "empty_dep", "dependencies": {"test": {"dependencies": {"test2": {}}}}})", 1),
        std::pair(R"({"name": "empty_dep", "dependencies": {"test": {"dependencies": {"test2": {}}},"test3": {}}})", 2)};

    for (const auto& a : map) {
      auto [str, size] = a;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = V1Parser::parse(const_cast<std::string&>(str));
      assert(dep.size() == size);
    }
  }

  void test_V1Parser_Substring() {
    std::vector<std::tuple<std::string, int, int, bool, std::string>> map = {
        std::make_tuple("abcdefghi", 0, 4, false, "abcde"),
        std::make_tuple("abcdefghi", 0, 4, true, "abcd"),
        std::make_tuple("{}}", 0, 1, false, "{}"),
    };
    for (const auto& a : map) {
      auto [source, from, to, cut, result] = a;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = V1Parser::substring(source, from, to, cut);
      assert(dep == result);
    }
  }

  void test_V1Parser_FindPairClose() {
    std::vector<std::tuple<std::string, int, char, char, int>> map = {
        std::make_tuple("{}", 0, '{', '}', 1),
        std::make_tuple("{}}", 0, '{', '}', 1),
        std::make_tuple("{a:{},b:{c:{}}}", 0, '{', '}', 14),
        std::make_tuple("{a:{},b:{c:{}}}", 3, '{', '}', 4),
        std::make_tuple("{a:{},b:{c:{}}}", 8, '{', '}', 13),
    };
    for (const auto& a : map) {
      auto [source, from, open, close, result] = a;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = V1Parser::findPairClose(source, from, open, close);
      assert(dep == result);
    }
  }

  void test_V1Parser_ParseValue() {
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> map = {
        std::make_tuple(R"({"key":value,"a":"b"})", "key", "fallback", "value"),
        std::make_tuple(R"({"key":"value","a":"b"})", "key", "fallback", "value"),
        std::make_tuple(R"({"key":"value","a":"b"})", "a", "fallback", "b"),
        std::make_tuple(R"({"key":"value","a":"b"})", "c", "fallback", "fallback"),
        std::make_tuple(R"({"key":"value","a":"b, {"c":"d"}"})", "c", "fallback", "fallback"),
    };
    for (const auto& a : map) {
      auto [source, key, fallback, result] = a;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = V1Parser::parseValue(source, key, fallback);
      assert(dep == result);
    }
  }

  void test_V1Parser() {
    test_V1Parser_Counts();
    test_V1Parser_Substring();
    test_V1Parser_FindPairClose();
    test_V1Parser_ParseValue();
  }

}  // namespace packagelock

auto main() -> int {
  packagelock::test_V1Parser();
  return 0;
};
