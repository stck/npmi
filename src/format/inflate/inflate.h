#ifndef INFLATE_H
#define INFLATE_H

#include "ifbstream.h"
#include "ringbuffer.h"
#include <iostream>
#include <vector>


namespace inflate {
  struct gzip_header;
  struct gzip_file;

  struct Node;
  struct Range;
  typedef int Code;
  typedef int Symbol;

  class Decodertype;
  class huffmantree;
  typedef huffmantree Decoder;

  Decoder build_decoder(const std::vector<Range>&);
  Symbol read_out(Decoder huffman_tree, ifbstream& in);

  std::pair<Decoder, Decoder> read_deflate_header(ifbstream& in);

  /* Decodes a inflate block into the output ostream,
   * also returning the last max_buffer_size bytes as a stream,
   * a buffer of the preceding max_buffer_size data is needed
   * for mid-stream blocks. */
  ringbuffer& inflate_block(ifbstream& in,
      std::ostream& output = std::cout,
      bool fixedcode       = false);
  ringbuffer& inflate_block(ifbstream& in, ringbuffer& buf,
      std::ostream& output = std::cout,
      bool fixedcode       = false);

  void gunzip(std::string fn, std::ostream& output = std::cout);

  namespace _UTIL {
    struct Coderow;

    std::vector<int> count_by_bitlength(
        const std::vector<inflate::Range>& ranges);

    template<class InputIterator>
    std::vector<inflate::Range> group_into_ranges(
        InputIterator first, InputIterator last);

    std::vector<inflate::Range> read_preheader(ifbstream& in);

    class teeprint;  // for debugging purposes
  }                  // namespace _UTIL
}  // namespace inflate


struct inflate::gzip_header {
  unsigned char id[2];
  unsigned char compression_method;
  unsigned char flags;
  unsigned char mtime[4];
  unsigned char extra_flags;
  unsigned char os;
};

struct inflate::gzip_file {
  gzip_header header;
  unsigned short xlen;
  unsigned char* extra;
  std::string fname;
  std::string fcomment;
  unsigned short crc16;
};

struct inflate::Node {
  Symbol symbol;  // -1 indicates inner node
  inflate::Node* zero;
  inflate::Node* one;
};

struct inflate::Range {  // Essentially a named std::pair
  int end;
  unsigned int bit_length;
};

struct inflate::_UTIL::Coderow {  // Essentially a named std::pair
  unsigned int bit_length;
  inflate::Code code;
};

class inflate::Decodertype {
public:
  virtual void insert(int codelen, inflate::Code, inflate::Symbol) = 0;
  virtual inflate::Symbol read_out(ifbstream& in) const            = 0;
  virtual inline bool empty() const noexcept                       = 0;
  virtual std::string str()                                        = 0;
};

// Decoder implementation
#include "huffmantree.h"

#ifdef DEBUG_INFGEN_OUTPUT
class inflate::_UTIL::teeprint {
public:
  bool wasliteral = false;
  bool quoted     = true;
  void operator()(teestream& out, ringbuffer& buf, char c);
};
#endif

#endif
