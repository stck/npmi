#ifndef NPM_REGEX_H
#define NPM_REGEX_H
#include <map>
#include <regex>
#include <string>
#include <vector>

namespace regex {
  using List                                           = std::vector<std::regex>;
  constexpr static std::string_view special_characters = "()[]{}?*+-|^$\\.&~# \t\n\r\v\f";
  static std::map<int, std::string> special_characters_map;


  namespace {
    void init_map() {
      for (auto& d : special_characters) {
        special_characters_map.insert(std::make_pair(static_cast<int>(d), std::string{"\\"} + std::string(1, d)));
      }
    }

    inline auto translate(const std::string& pattern) -> std::string {
      std::string result_string = "^.*";

      if (special_characters_map.empty()) {
        init_map();
      }

      for (auto c : pattern) {
        if (c == '*') {
          result_string.append(".*");
        } else if (c == '?') {
          result_string.append(".?");
        } else if (special_characters.find(c) != std::string::npos) {
          result_string.append(special_characters_map[static_cast<int>(c)]);
        } else {
          result_string += c;
        }
      }
      result_string.append(R"($)");

      return result_string;
    }
  }  // namespace


  inline auto convert(const std::vector<std::string>& list) -> List {
    List _list;
    for (const auto& i : list) {
      std::string pattern = translate(i);
      std::regex re(pattern, std::regex::icase);

      _list.emplace_back(re);
    }

    return _list;
  }

  inline auto test(const std::string& source, const List& target) -> bool {
    bool matched = false;
    for (const auto& j : target) {
      if (std::regex_match(source, j)) {
        matched = true;
        break;
      }
    }

    return matched;
  }

}  // namespace regex

#endif  //NPM_REGEX_H
