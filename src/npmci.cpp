#include <fstream>
#include <vector>

#include "format/gzip/decompressor.h"
#include "format/package_lock.hpp"
#include "format/tar.hpp"
#include "proto/http.hpp"
#include "util/args.hpp"
#include "util/fs.hpp"
#include "util/regex.h"
#include "util/thread_pool.hpp"

void create_fs(const std::string& prefix, const regex::List& list, const tar::Content& files) noexcept {
  for (const auto& i : files) {
    if (!regex::test(i.first, list)) {
      std::string path = "." + prefix + "/" + i.first;

      fs::create_dir(path, false);

      std::ofstream out(path);
      out << i.second;
      out.close();
    }
  }
}

auto inflate(const http::response& response) -> unsigned char* {
  try {
    size_t deflatedBufferSize = response.content.size();  // response.size;
    size_t inflatedBufferSize = 20000000;
    auto* deflatedData        = static_cast<const char*>(response.content.data());
    auto* inflatedData        = new unsigned char[inflatedBufferSize];

    unsigned int inflatedSize = Decompressor::Feed(deflatedData, deflatedBufferSize, inflatedData, inflatedBufferSize, false);
    if (inflatedSize == -1) {
      std::cout << "decompression error!" << std::endl;
      return new unsigned char[1];
    } else {
      return inflatedData;
    }
  } catch (std::exception& e) {
    std::cout << e.what() << std::endl;
    return new unsigned char[1];
  }
}

auto untar(unsigned char* from) {
  if (from[0] == '\0') return tar::Content{};
  return tar::read(from, "package/");
}

auto download(const std::string& from) {
  // todo: reuse existing connections

  std::string a    = from.substr(from.find("://") + 3);
  std::string host = a.substr(0, a.find_first_of('/'));
  std::string path = a.substr(a.find_first_of('/'));

  http::client cli{host};
  return cli.download(path);
}

auto main(int argc, char* argv[]) -> int {
  args::parse(argc, argv);

  const bool include_dev = args::get("dev", false);
  const bool include_opt = args::get("optional", false);
  const bool verbose     = args::get("verbose", false);
  const std::string file = "package-lock.json";

  std::vector<std::string> template_list = fs::read_ignore(".pkgignore");
  regex::List list                       = regex::convert(template_list);

  try {
    std::string jsonStr;
    try {
      jsonStr = fs::read_file(file, true);
    } catch (std::runtime_error& e) {
      std::cerr << e.what() << std::endl;

      return 1;
    }

    Dependencies cleanedDependencies{};
    auto dependencies = package_lock::parse(jsonStr);
    std::copy_if(dependencies.begin(), dependencies.end(), std::back_inserter(cleanedDependencies), [include_dev, include_opt](Dependency i) {
      return (include_dev && i.dev) || (include_opt && i.optional) || (!include_opt && !include_dev);
    });

    if (cleanedDependencies.empty()) {
      std::cerr << "No entries to install" << std::endl;
      return 1;
    }

    ThreadPool tp(std::thread::hardware_concurrency());

    for (auto& a : cleanedDependencies) {
      tp.enqueue([verbose](const Dependency& _a, const regex::List& _b) {
        verbose&& std::cout << "downloading: " << _a.resolved << std::endl;
        auto c = download(_a.resolved);
        verbose&& std::cout << "inflating: " << _a.resolved << std::endl;
        auto* d = inflate(c);
        verbose&& std::cout << "untar: " << _a.resolved << std::endl;
        auto e = untar(d);
        verbose&& std::cout << "create_fs: " << _a.path << std::endl;
        create_fs(_a.path, _b, e);
      },
          a, list);
    }
  } catch (const std::exception& e) {
    std::cout << "error main " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
