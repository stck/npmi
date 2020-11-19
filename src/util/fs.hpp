#include <map>
#include <regex>
#include <string>
#include <sys/stat.h>  // stat
#include <vector>
#if defined(_WIN32)
#  include <direct.h>  // _mkdir
#endif

namespace fs {
  class FileException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

  std::vector<std::string> blacklist_templates = {  // NOLINT(cert-err58-cpp)
      "licen?e*",
      "authors*",
      "readme*",
      "funding*",
      "changelog*",
      "*.m?d",
      "*.markdown",
      "*.workflow",
      "/.*",
      "/__mocks__*",
      "*.spec.js",
      "*.html",
      "*.txt"};


  namespace {
    auto split(const std::string& content, char sep, char comment) -> std::vector<std::string> {
      std::vector<std::string> output;
      std::string::size_type ppos = 0;
      std::string::size_type pos  = 0;

      while ((pos = content.find(sep, pos)) != std::string::npos) {
        std::string line(content.substr(ppos, pos - ppos));

        if (line[0] != comment) {
          output.emplace_back(line);
        }

        pos++;
        ppos = pos;
      }

      return output;
    }

    auto split_fill(const std::string& s, char separator) -> std::vector<std::string> {
      std::vector<std::string> output;
      std::string::size_type pos = 0;

      while ((pos = s.find(separator, pos)) != std::string::npos) {
        std::string substring(s.substr(0, pos));
        if (!substring.empty()) {
          output.push_back(substring);
        }
        ++pos;
      }

      output.push_back(s.substr(0, pos));

      return output;
    }
  }  // namespace

  void create_dir(const std::string& path, bool createLast) {
    auto tokens = split_fill(path, '/');
    if (!createLast) {
      tokens.erase(tokens.end() - 1, tokens.end());
    }

    for (auto const& e : tokens) {
#if defined(_WIN32)
      _mkdir(e.c_str());
#else
      mode_t mode = 0755;
      mkdir(e.c_str(), mode);
#endif
    }
  }

  auto read_file(const std::string& path, bool should_exist = false) -> std::string {
    std::ifstream i(path);
    if (!i.is_open()) {
      i.close();

      if (should_exist) {
        throw FileException("Unable to open file '" + path + "'");
      }
      return "";
    }

    std::string content((std::istreambuf_iterator<char>(i)), (std::istreambuf_iterator<char>()));
    i.close();
    return content;
  }

  auto read_ignore(const std::string& path) -> std::vector<std::string> {
    auto content = read_file(path, false);

    if (content.empty()) {
      return blacklist_templates;
    } else {
      return split(content, '\n', '#');
    }
  }

}  // namespace fs
