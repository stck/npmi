#ifndef NPM_PACKAGE_LOCK_HPP
#define NPM_PACKAGE_LOCK_HPP

#include "../headers/dependency.h"
#include <string>

namespace package_lock {
#define OS '{'
#define OE '}'
#define OKP ':'
#define OK '"'
#define OD ','
#define DEP "dependencies"
#define NM "/node_modules/"
#define LFV "lockfileVersion"
#define PACK "packages"

  namespace {
    inline const std::vector<char> blacklist{'\t', '\n', '\r', ' '};

    class UnsupportedLockfileVersionException : public std::runtime_error {
    public:
      using std::runtime_error::runtime_error;
    };

    class LockfileVersionMissingException : public std::runtime_error {
    public:
      using std::runtime_error::runtime_error;
    };

    /**
     * Find FIRST block START index by given open/close chars
     * @param source Content to search for
     * @param fromIndex Start searching by given index
     * @param pairOpen Char that represent block open
     * @param pairClose Char that represents block close
     * @return index of block start in source
     */
    auto findStart(const std::string& source, const size_t fromIndex, const char pairOpen, [[maybe_unused]] const char pairClose) -> size_t {
      return source.find_first_of(pairOpen, fromIndex);
    }

    /**
     * Find FIRST block END index by given open/close chars
     * @param source Content to search for
     * @param fromIndex Start searching by given index
     * @param pairOpen Char that represent block open
     * @param pairClose Char that represents block close
     * @return index of block end in source
     */
    auto findClose(const std::string& source, const size_t fromIndex, const char pairOpen, const char pairClose) -> size_t {
      int level      = 1;
      size_t lastPos = fromIndex;
      while (level > 0) {
        size_t nextPos    = lastPos + 1;
        size_t firstStart = source.find_first_of(pairOpen, nextPos);
        size_t firstEnd   = source.find_first_of(pairClose, nextPos);

        if (firstStart != std::string::npos && firstEnd != std::string::npos) {
          if (firstStart < firstEnd) {
            level += 1;
            lastPos = firstStart;
          } else {
            level -= 1;
            lastPos = firstEnd;
          }
        } else {
          if (firstEnd != std::string::npos) {
            lastPos = firstEnd;
            level -= 1;
          }
        }
      }

      return lastPos;
    }

    /**
     * Substring from/to
     * @param source Content to perform substring
     * @param from Index of substring beginning
     * @param to Index of substring ending
     * @param cutLast Should the last char be omitted
     * @return Substring by given range
     */
    auto substring(const std::string& source, size_t from, size_t to, bool cutLast) -> std::string {
      if (to == source.length() - 1) return source.substr(from);
      std::string result = source.substr(from, to - from + (cutLast ? 0 : 1));
      return result;
    }
  }  // namespace

  auto findStart(const std::string& source, const size_t fromIndex) -> size_t {
    return findStart(source, fromIndex, OS, OE);
  }

  auto findClose(const std::string& source, const size_t fromIndex) -> size_t {
    return findClose(source, fromIndex, OS, OE);
  }

  auto substring(const std::string& source, size_t from, size_t to) -> std::string {
    return substring(source, from, to, false);
  }

  auto getClosestBlock(const std::string& content, const size_t pos) -> std::string {
    size_t start(findStart(content, pos));
    size_t end(findClose(content, start));
    std::string block(substring(content, start + 1, end, true));

    return block;
  }

  auto parseKey(const std::string& source) -> std::string {
    std::string ignore{OK, OD};

    size_t keyStart = source.find_first_not_of(ignore);
    size_t keyEnd   = source.find_first_of(OK, keyStart);

    return substring(source, keyStart, keyEnd, true);
  }

  auto parseValue(const std::string& source, const std::string& key, const std::string& fallback) -> std::string {
    size_t firstOS  = findStart(source, 1);
    size_t keyStart = source.find(key);
    if (keyStart != std::string::npos && (keyStart < firstOS || firstOS == std::string::npos)) {
      std::string ignore{OK, OD, OE};
      size_t valueStart = source.find_first_not_of(OK, source.find_first_of(OKP, keyStart) + 1);
      size_t valueEnd   = static_cast<size_t>(source.find_first_of(ignore, valueStart)) - 1;
      return substring(source, valueStart, valueEnd, false);
    }

    return fallback;
  }

  auto parseDependency(const std::string& dependency, const std::string& overridePath) -> Dependency {
    std::string key{parseKey(dependency)};
    std::string value{getClosestBlock(dependency, 0)};

    return Dependency{
        .path     = overridePath.empty() ? key : overridePath + key,
        .resolved = parseValue(value, "resolved", ""),
        .dev      = parseValue(value, "dev", "false") == "true",
        .optional = parseValue(value, "optional", "false") == "true",
    };
  }

  auto parseDependencies(const std::string& source, const std::string& root) -> Dependencies {  // NOLINT(misc-no-recursion)
    size_t depPos = source.find(root.empty() ? PACK : DEP, 0);

    if (depPos == std::string::npos) {
      return {};
    }

    size_t start      = findStart(source, depPos);
    size_t end        = findClose(source, start);
    std::string pairs = substring(source, start + 1, end, true);

    Dependencies dependencies;
    size_t lastPos = 0;
    while (findStart(pairs, lastPos) != std::string::npos) {
      size_t firstOS = findStart(pairs, lastPos);
      size_t lastOS  = findClose(pairs, firstOS);

      std::string pair = substring(pairs, lastPos, lastOS + 1, true);
      lastPos += pair.length();

      if (pair[0] == OK && pair[1] == OK) continue;
      Dependency dep{parseDependency(pair, root)};
      dependencies.emplace_back(dep);

      if (!root.empty()) {
        std::string appendPath = root.empty() ? root : dep.path + NM;
        Dependencies childDep  = parseDependencies(pair, appendPath);

        if (!childDep.empty()) {
          dependencies.insert(dependencies.end(), childDep.begin(), childDep.end());
        }
      }
    }

    return dependencies;
  }

  auto parse(std::string& content) -> Dependencies {
    for (auto& blChar : blacklist) {
      content.erase(std::remove(content.begin(), content.end(), blChar), content.end());
    }

    if (content.empty()) {
      return Dependencies{};
    }

    std::string value{parseValue(content, LFV, "-1")};
    int version = std::stoi(value);

    switch (version) {
      case -1:
        throw LockfileVersionMissingException{"Lock file version wasn't found in package-lock.json"};
      case 1:
      case 2:
        return parseDependencies(getClosestBlock(content, 0), version == 1 ? NM : "");
      default:
        throw UnsupportedLockfileVersionException{"Lockfile version is not supported"};
    }
  };


#undef OS
#undef OE
#undef OKP
#undef OK
#undef OD
#undef DEP
#undef NM
#undef LFV
#undef PACK
}  // namespace package_lock

#endif  // NPM_PACKAGE_LOCK_HPP