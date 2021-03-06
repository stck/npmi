#include "../headers/tar_header.h"
#include <string>
#include <vector>

namespace tar {
#define HEADER_SIZE 512
#define PADDING_SIZE 512
#define ASCII_TO_NUMBER(num) ((num) -48)

  namespace {
    auto decodeString(const char* in, size_t len) -> std::string {
      std::string a;
      for (int j = 0; j < len; j++) {
        if (in[j] == 0) {
          break;
        }
        a += in[j];
      }

      return a;
    }

    auto decodeOctal(char* data, size_t size = 12) -> uint64_t {
      unsigned char* currentPtr  = reinterpret_cast<unsigned char*>(data) + size;
      uint64_t sum               = 0;
      uint64_t currentMultiplier = 1;
      unsigned char* checkPtr    = currentPtr;
      for (; checkPtr >= reinterpret_cast<unsigned char*>(data); checkPtr--) {
        if ((*checkPtr) == 0 || (*checkPtr) == ' ') {
          currentPtr = checkPtr - 1;
        }
      }
      for (; currentPtr >= reinterpret_cast<unsigned char*>(data); currentPtr--) {
        sum += ASCII_TO_NUMBER(*currentPtr) * currentMultiplier;
        currentMultiplier *= 8;
      }
      return sum;
    }
  }  // namespace

  using Content = std::vector<std::pair<std::string, std::string>>;

  auto read(unsigned char* file, const std::string& prefix) -> Content {
    Content tc;

    unsigned int i = 0;
    for (;;) {
      auto* header          = reinterpret_cast<Header*>(&file[i]);
      unsigned int fileSize = decodeOctal(header->fileSize, 12);
      std::string fileName  = decodeString(header->fileName, 100);

      if (!prefix.empty() && fileName.find(prefix) == 0) {  // NOLINT(abseil-string-find-startswith)
        fileName = fileName.substr(prefix.length());
      }

      if (fileName.empty()) {
        break;
      }
      std::string fileContent = decodeString(&header->firstContent, fileSize);
      tc.emplace_back(std::pair<std::string, std::string>(fileName, fileContent));

      unsigned int contentSize = (fileSize % PADDING_SIZE == 0) ? fileSize : ((fileSize / PADDING_SIZE) + 1) * PADDING_SIZE;
      i += HEADER_SIZE + contentSize;
    }

    return tc;
  }
}  // namespace tar
