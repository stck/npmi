#ifndef NPM_PACKAGE_LOCK_HPP
#define NPM_PACKAGE_LOCK_HPP

#include "../headers/dependency.h"
#include <algorithm>
#include <string>
#include <vector>

namespace packagelock {
#define OS '{'
#define OE '}'
#define OKP ':'
#define OK '"'
#define OD ','
#define DEP "dependencies"
#define NM "/node_modules/"

  class V1Parser {
  private:
    static std::string substring(const std::string& source, int from, int to) {
      return substring(source, from, to, false);
    }

    static std::string substring(const std::string& source, int from, int to, bool cutLast) {
      if (to == source.length() - 1) return source.substr(from);
      std::string result = source.substr(from, to - from + (cutLast ? 0 : 1));
      return result;
    }

    static int findPairClose(const std::string& source, const int fromIndex, const char pairOpen, const char pairClose) {
      int level    = 1;
      int iterDone = 0;
      int lastPos  = fromIndex;
      while (level > 0) {
        iterDone       = iterDone + 1;
        int nextPos    = lastPos + 1;
        int firstStart = source.find_first_of(pairOpen, nextPos);
        int firstEnd   = source.find_first_of(pairClose, nextPos);

        if (firstStart != std::string::npos && firstEnd != std::string::npos) {
          if (firstStart < firstEnd) {
            level   = level + 1;
            lastPos = firstStart;
          } else {
            level   = level - 1;
            lastPos = firstEnd;
          }
        } else {
          if (firstEnd == std::string::npos) {
            break;  //WTF?? wrong syntax
          } else {
            lastPos = firstEnd;
            level   = level - 1;
          }
        }
      }

      return lastPos;
    }

  protected:
    static std::string parseValue(const std::string& source, const std::string& key, const std::string& fallback) {
      int firstOS  = source.find_first_of(OS, 1);
      int keyStart = source.find(key);
      if (keyStart != std::string::npos && (keyStart < firstOS || firstOS == std::string::npos)) {
        std::string ignore;
        ignore += OK;
        ignore += OD;
        ignore += OE;

        int valueStart = source.find_first_not_of(OK, source.find_first_of(OKP, keyStart) + 1);
        int valueEnd   = int(source.find_first_of(ignore, valueStart)) - 1;

        std::string a = substring(source, valueStart, valueEnd, false);
        return a;
      }

      return fallback;
    }

    static Dependency parseDependency(const std::string& root, const std::string& dep) {  // NOLINT(misc-no-recursion)
      std::string ignore;
      ignore += OK;
      ignore += OD;

      int keyStart      = dep.find_first_not_of(ignore);
      int keyEnd        = dep.find_first_of(OK, keyStart);
      std::string key   = substring(dep, keyStart, keyEnd, true);
      std::string value = dep.substr(dep.find_first_of(OS, keyEnd));

      std::string newRoot = root + key;
      return Dependency{
          .path         = newRoot,
          .resolved     = parseValue(value, "resolved", ""),
          .dev          = parseValue(value, "dev", "false") == "true",
          .optional     = parseValue(value, "optional", "false") == "true",
          .dependencies = parseDependencies(newRoot + NM, value),
      };
    }

    static Dependencies parseDependencies(const std::string& root, const std::string& i) {  // NOLINT(misc-no-recursion)
      Dependencies dependencies;
      int depPos = i.find(DEP, 0);

      if (depPos == std::string::npos) {
        return dependencies;
      }

      int start = i.find_first_of(OS, depPos);

      std::string pairs = substring(i, start + 1, findPairClose(i, start, OS, OE), true);

      int lastPos = 0;
      while (pairs.find_first_of(OS, lastPos) != std::string::npos) {
        int firstOS = pairs.find_first_of(OS, lastPos);
        int lastOS  = findPairClose(pairs, firstOS, OS, OE);

        std::string pair = substring(pairs, lastPos, lastOS + 1, true);
        lastPos += pair.length();
        dependencies.emplace_back(parseDependency(root, pair));
      }

      return dependencies;
    }

  public:
    static Dependencies parse(std::string& i) {
      std::vector<char> delChr{'\t', '\n', '\r', ' '};
      int start(i.find_first_of(OS));
      int end(i.find_last_of(OE));

      for (auto& del : delChr) {
        i.erase(std::remove(i.begin(), i.end(), del), i.end());
      }
      if (i.empty()) {
        return Dependencies{};
      }

      std::string rootObject = substring(i, start, end);
      return parseDependencies(NM, rootObject);
    }

#ifdef UNITTEST
    friend void test_V1Parser_Substring();
    friend void test_V1Parser_FindPairClose();
    friend void test_V1Parser_ParseValue();
#endif
  };
}  // namespace packagelock

#endif  //NPM_PACKAGE_LOCK_HPP
