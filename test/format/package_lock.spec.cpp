#include "../../src/format/package_lock.hpp"
#include <cassert>
#include <tuple>


namespace package_lock {
  auto test_findStart() {
    std::vector<std::tuple<std::string, int, char, char, int>> testSuite = {
        std::make_tuple("{}", 0, '{', '}', 0),
        std::make_tuple("{}}", 0, '{', '}', 0),
        std::make_tuple("{a:{},b:{c:{}}}", 1, '{', '}', 3),
        std::make_tuple("{a:{},b:{c:{}}}", 4, '{', '}', 8),
        std::make_tuple("{a:{},b:{c:{}}}", 10, '{', '}', 11),
    };
    for (const auto& testCase : testSuite) {
      auto [source, from, open, close, result] = testCase;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = findStart(source, from, open, close);
      assert(dep == result);
    }
  }

  auto test_findClose() {
    std::vector<std::tuple<std::string, int, char, char, int>> testSuite = {
        std::make_tuple("{}", 0, '{', '}', 1),
        std::make_tuple("{}}", 0, '{', '}', 1),
        std::make_tuple("{a:{},b:{c:{}}}", 0, '{', '}', 14),
        std::make_tuple("{a:{},b:{c:{}}}", 3, '{', '}', 4),
        std::make_tuple("{a:{},b:{c:{}}}", 8, '{', '}', 13),
    };
    for (const auto& testCase : testSuite) {
      auto [source, from, open, close, result] = testCase;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = findClose(source, from, open, close);
      assert(dep == result);
    }
  }

  auto test_substring() {
    std::vector<std::tuple<std::string, int, int, bool, std::string>> testSuite = {
        std::make_tuple("abcdefghi", 0, 4, false, "abcde"),
        std::make_tuple("abcdefghi", 0, 4, true, "abcd"),
        std::make_tuple("{}}", 0, 1, false, "{}"),
    };
    for (const auto& testCase : testSuite) {
      auto [source, from, to, cut, result] = testCase;  // NOLINT(performance-unnecessary-copy-initialization)

      std::string dep;
      if (cut) {
        dep = substring(source, from, to, cut);
      } else {
        dep = substring(source, from, to);
      }

      assert(dep == result);
    }
  }

  auto test_getClosestBlock() {
    std::vector<std::tuple<std::string, int, std::string>> testSuite = {
        std::make_tuple(R"({"foo":"bar","baz":{"test":"test"}})", 14, R"("test":"test")"),
        std::make_tuple("{{}}}", 0, "{}"),
    };
    for (const auto& testCase : testSuite) {
      auto [source, from, result] = testCase;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = getClosestBlock(source, from);
      assert(dep == result);
    }
  }

  auto test_parseValue() {
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> testSuite = {
        std::make_tuple(R"({"key":value,"a":"b"})", "key", "fallback", "value"),
        std::make_tuple(R"({"key":"value","a":"b"})", "key", "fallback", "value"),
        std::make_tuple(R"({"key":"value","a":"b"})", "a", "fallback", "b"),
        std::make_tuple(R"({"key":"value","a":"b"})", "c", "fallback", "fallback"),
        std::make_tuple(R"({"key":"value","a":"b, {"c":"d"}"})", "c", "fallback", "fallback"),
    };
    for (const auto& testCase : testSuite) {
      auto [source, key, fallback, result] = testCase;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = parseValue(source, key, fallback);
      assert(dep == result);
    }
  }

  auto test_parseDependency() {
    std::vector<std::tuple<std::string, std::string, std::string, std::string, bool, bool>> testSuite = {
        std::make_tuple(R"("foo":{"resolved":"bar","optional":true})", "/foo_bar/", "/foo_bar/foo", "bar", false, true),
        std::make_tuple(R"("foo":{"resolved":"bar","dev":true})", "/foo_bar/", "/foo_bar/foo", "bar", true, false),
        std::make_tuple(R"("foo":{"resolved":"bar","dependencies": {"baz":"^1.0.0"}})", "/foo_bar/", "/foo_bar/foo", "bar", false, false),
        std::make_tuple(R"("foo_bar/foo":{"resolved":"bar","dependencies":{"baz":"^1.0.0"}})", "", "foo_bar/foo", "bar", false, false),
        std::make_tuple(R"("foo_bar/foo":{"resolved":"bar","dev":true})", "", "foo_bar/foo", "bar", true, false),
    };
    for (const auto& testCase : testSuite) {
      auto [source, overridePath, path, resolved, dev, optional] = testCase;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = parseDependency(source, overridePath);

      assert(dep.path == path);
      assert(dep.resolved == resolved);
      assert(dep.dev == dev);
      assert(dep.optional == optional);
    }
  }

  auto test_parseDependencies() {
    std::vector<std::tuple<std::string, std::string, std::string, std::string, int>> testSuite = {
        std::make_tuple(R"({"name":"foo","dependencies":{"foo":{"resolved":"http://example.com/","dependencies":{"baz":{"resolved":"http://example2.com/"}}},"baz":{"resolved":"http://example3.com/"}}})", "/node_modules/", "/node_modules/foo", "/node_modules/foo/node_modules/baz", 3),
        std::make_tuple(R"({"name":"empty_dep","packages":{"": {},"/node_modules/test":{"dependencies":{"test2":{}}},"/node_modules/test/node_modules/foo":{}}})", "", "/node_modules/test", "/node_modules/test/node_modules/foo", 2),
        std::make_tuple(R"({"name":"empty_dep","packages":{"/node_modules/test":{"dependencies":{"test2":{}}},"/node_modules/foo":{}}})", "", "/node_modules/test", "/node_modules/foo", 2),
    };

    for (const auto& testCase : testSuite) {
      auto [content, root, firstDepPath, secondDepPath, depCount] = testCase;

      auto dependencies = parseDependencies(content, root);

      assert(dependencies.size() == depCount);
      assert(dependencies.at(0).path == firstDepPath);
      assert(dependencies.at(1).path == secondDepPath);
    }
  }

  auto test_V1Counts() {
    std::vector<std::pair<std::string, int>> testSuite = {
        std::pair(R"({"name": "empty_lock", "lockfileVersion": 1})", 0),
        std::pair(R"({"name": "empty_dep", "lockfileVersion": 1, "dependencies": {}})", 0),
        std::pair(R"({"name": "empty_dep", "lockfileVersion": 1, "dependencies": {"test": {}}})", 1),
        std::pair(R"({"name": "empty_dep", "lockfileVersion": 1, "dependencies": {"test": {"dependencies": {"test2": {}}}}})", 2),
        std::pair(R"({"name": "empty_dep", "lockfileVersion": 1, "dependencies": {"test": {"dependencies": {"test2": {}}},"test3": {}}})", 3)};

    for (const auto& testCase : testSuite) {
      auto [str, size] = testCase;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = parse(const_cast<std::string&>(str));
      assert(dep.size() == size);
    }
  }

  auto test_V2Counts() {
    std::vector<std::pair<std::string, int>> testSuite = {
        std::pair(R"( )", 0),
        std::pair(R"({"name": "empty_lock", "lockfileVersion": 2})", 0),
        std::pair(R"({"name": "empty_dep", "lockfileVersion": 2, "packages": {}})", 0),
        std::pair(R"({"name": "empty_dep", "lockfileVersion": 2, "packages": {"test": {}}})", 1),
        std::pair(R"({"name": "empty_dep", "lockfileVersion": 2, "packages": {"test": {"dependencies": {"test2": {}}}}})", 1),
        std::pair(R"({"name": "empty_dep", "lockfileVersion": 2, "dependencies": {"test": {"dependencies": {"test2": {}}},"test3": {}}})", 0)};

    for (const auto& testCase : testSuite) {
      auto [str, size] = testCase;  // NOLINT(performance-unnecessary-copy-initialization)

      auto dep = parse(const_cast<std::string&>(str));
      assert(dep.size() == size);
    }
  }

  auto test_Counts() {
    std::vector<std::pair<std::string, int>> testSuite = {
        std::pair(R"({"name": "empty_lock"})", 0),
        std::pair(R"({"name": "empty_lock", "lockfileVersion": 3})", 0),
    };

    for (const auto& testCase : testSuite) {
      auto [str, size]     = testCase;  // NOLINT(performance-unnecessary-copy-initialization)
      auto exceptionRaised = false;

      try {
        auto dep = parse(const_cast<std::string&>(str));
      } catch (std::exception& e) {
        exceptionRaised = true;
      }
      assert(exceptionRaised);
    }
  }

  void test_baseFunctions() {
    test_findStart();
    test_findClose();
    test_substring();
    test_getClosestBlock();
    test_parseValue();
    test_parseDependency();
    test_parseDependencies();
  }

  auto test_V1Parser() {
    test_V1Counts();
  }

  auto test_V2Parser() {
    test_V2Counts();
  }

  auto test_parser() {
    test_Counts();
    test_V1Parser();
    test_V2Parser();
  }

}  // namespace package_lock

auto main() -> int {
  package_lock::test_baseFunctions();
  package_lock::test_parser();
  return 0;
};
