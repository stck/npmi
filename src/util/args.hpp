#include <map>
#include <string>
#include <tuple>


namespace args {
  inline std::map<std::string, bool> _args;

  inline bool get(const std::string& key, bool fallback = false) {
    if (_args.find(key) != _args.end()) {
      return _args.at(key);
    } else {
      return fallback;
    }
  }

  inline void parse(const int argc, const char* const* argv) {
    for (int i = 0; i < argc; i++) {
      std::string str{argv[i]};
      if (str.find_first_not_of('-') != std::string::npos) {
        _args.emplace(str.substr(2), true);
      }
    }
  }
};  // namespace args
