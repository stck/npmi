#ifndef NPM_DEPENDENCY_H
#define NPM_DEPENDENCY_H

#include <string>
#include <vector>

struct Dependency {
  std::string path;
  std::string resolved;
  bool dev{true};
  bool optional{true};
  std::vector<Dependency> dependencies{};
};

using Dependencies = std::vector<Dependency>;

#endif  //NPM_DEPENDENCY_H
