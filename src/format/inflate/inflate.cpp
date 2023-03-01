#include "inflate.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "ifbstream.h"
#include "ringbuffer.h"
#include "teestream.h"


namespace inflate {
  namespace flag {
    const unsigned char text    = 0x01;
    const unsigned char hcrc    = 0x02;
    const unsigned char extra   = 0x04;
    const unsigned char fname   = 0x08;
    const unsigned char comment = 0x10;
  }  // namespace flag

  const std::vector<inflate::Range> fixedranges = {
      (inflate::Range){143, 8},
      (inflate::Range){255, 9},
      (inflate::Range){279, 7},
      (inflate::Range){287, 8}};

  const int max_buffer_size = 32768;
}  // namespace inflate


std::vector<int> inflate::_UTIL::count_by_bitlength(
    const std::vector<inflate::Range>& ranges) {
  /* Calculates the length of each canonical huffman code range */
  std::vector<int> lengths;
  int max_bit_length;
  std::vector<inflate::Range> diff;

  // max = max(ranges, lambda x, y: x.bit_length < y.bit_length)
  max_bit_length = std::max_element(ranges.begin(), ranges.end(),
      [](const inflate::Range& x, const inflate::Range& y) {
        return x.bit_length < y.bit_length;
      })->bit_length;

  // count the lengths of each range
  // take the difference between previous and current range ends
  // diff = map(zip(ranges, [0] + ranges[:-1]), lambda x, y: x.end - y.end)
  std::adjacent_difference(ranges.begin(), ranges.end(),
      back_inserter(diff),
      [](const inflate::Range& p, const inflate::Range& c) {
        return (inflate::Range){p.end - c.end, p.bit_length};
      });
  diff[0].end += 1;  // 0 is a symbol
  // collect range lengths into bins by code bit_length
  std::fill_n(back_inserter(lengths), max_bit_length + 1, 0);
  for (auto& drange : diff) {
    lengths[drange.bit_length] += drange.end;
  }
  return lengths;
}


inflate::Decoder inflate::build_decoder(
    const std::vector<inflate::Range>& ranges) {
  /* Build a decoder from the canonical huffman code ranges */
  inflate::Decoder decoder;
  if (ranges.empty()) return decoder;

  std::vector<int> numrows;
  std::vector<inflate::Code> nextcodes;
  std::vector<inflate::_UTIL::Coderow> codebook;

  numrows = inflate::_UTIL::count_by_bitlength(ranges);

  // determine starting codes for each bit length
  nextcodes.push_back(0);  // start at 0
  // acc = 0
  // for num in numrows[1:]:
  //      acc = (acc + num) << 1
  //      nextcodes.append(acc if num else 0)
  int acc = 0;
  for (auto i = numrows.begin() + 1, e = numrows.end(); i < e; i++) {
    nextcodes.push_back(*i ? acc : 0);
    acc = (acc + *i) << 1;
  }

  // populate the codebook
  // codebook = map(zip(diff, ranges),
  //                lambda d, r: zip(d*[r.code], range(r.end-d, r)
  //                                             if r.code != 0 else 0))
  codebook.reserve(ranges.back().end);
  auto appendtobook = back_inserter(codebook);
  for (const auto& range : ranges) {
    unsigned int len = range.bit_length;
    std::generate_n(appendtobook, range.end + 1 - codebook.size(),
        [&nextcodes, &len]() {
          return inflate::_UTIL::Coderow(
              {len, (len ? nextcodes[len]++ : 0)});
        });
  }

  // invert codebook into decoder
  inflate::Symbol symbol = 0;
  for (auto& row : codebook) {
    if (row.bit_length < 1) {  // skip empty lines
      symbol++;
    } else {
      decoder.insert(row.bit_length, row.code, symbol++);
    }
  }
  return decoder;
}

template<class InputIterator>
std::vector<inflate::Range> inflate::_UTIL::group_into_ranges(
    InputIterator first, InputIterator last) {
  /* Groups vectors of code lengths into canonical huffman ranges */
  std::vector<inflate::Range> ranges;

  // collapse into ranges (possibly unnecessary?)
  // may use Eric Niebler's range lib group_by in future STL
  InputIterator it = first;
  for (int i = 0; it < last; i++, it++) {
    if (*it == *(it + 1) && it < last - 1) {
      continue;  // only push when code bit length changes or at end
    }
    ranges.emplace_back(
        (inflate::Range){i, *it});
  }
  return ranges;
}


std::vector<inflate::Range> inflate::_UTIL::read_preheader(ifbstream& in) {
  int hclen;
  hclen = in.read(4);
  std::vector<unsigned int> preheader_lengths(19, 0);  // input
  std::vector<inflate::Range> preheader_ranges;        // output
  static const int preheader_offsets[] = {             // according to spec
      16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
  // read in offsets
  for (int i = 0; i < (hclen + 4); i++) {
    preheader_lengths[preheader_offsets[i]] = in.read(3);
  }

  preheader_ranges = inflate::_UTIL::group_into_ranges(
      preheader_lengths.begin(), preheader_lengths.end());
  return preheader_ranges;
}


std::pair<inflate::Decoder, inflate::Decoder>
inflate::read_deflate_header(ifbstream& in) {
  std::vector<inflate::Range> preheader_ranges;
  inflate::Decoder preheader_dec;
  inflate::Decoder literals_dec, distance_dec;

  unsigned int bit_length;
  std::vector<unsigned int> lengths;
  auto into_lengths = back_inserter(lengths);

  int hlit, hdist;
  hlit  = in.read(5);
  hdist = in.read(5);
  lengths.reserve(hlit + hdist + 258);

  preheader_ranges = inflate::_UTIL::read_preheader(in);
  preheader_dec    = build_decoder(preheader_ranges);


  for (inflate::Symbol symbol = 0; symbol < (hlit + hdist + 258); symbol++) {
    try {
      bit_length = preheader_dec.read_out(in);
    } catch (std::invalid_argument& e) {
      throw std::invalid_argument(
          "Preheader Code Invalid: unindexed code");
    }
    if (bit_length > 15) {  // repeat symbol
      int repeat;
      switch (bit_length) {
        case 16:
          repeat = in.read(2) + 3;
          std::fill_n(into_lengths, repeat, lengths.back());
          break;
        case 17:
          repeat = in.read(3) + 3;
          std::fill_n(into_lengths, repeat, 0);
          break;
        case 18:
          repeat = in.read(7) + 11;
          std::fill_n(into_lengths, repeat, 0);
          break;
        default:
          throw std::runtime_error(
              "Preheader Decoder Invalid: symbol too large");
      }
      symbol += repeat - 1;
    } else {  // read_out output guaranteed > 0
      into_lengths = bit_length;
    }
  }
  auto literals_ranges = inflate::_UTIL::group_into_ranges(
      lengths.begin(), lengths.begin() + (hlit + 257));

  literals_dec = inflate::build_decoder(literals_ranges);

  auto distance_ranges = inflate::_UTIL::group_into_ranges(
      lengths.begin() + (hlit + 257), lengths.end());


  distance_dec = inflate::build_decoder(distance_ranges);

  return std::make_pair(literals_dec, distance_dec);
}

auto inflate::inflate_block(ifbstream& in,
    std::ostream& output /*=std::cout*/, bool fixedcode /*=false*/) -> ringbuffer& {
  ringbuffer buf(inflate::max_buffer_size);
  return inflate::inflate_block(in, buf, output, fixedcode);
}

auto inflate::inflate_block(ifbstream& in,
    ringbuffer& buf,
    std::ostream& output /*=std::cout*/,
    bool fixedcode /*=false*/) -> ringbuffer& {
  teestream teeout(output, buf);

  static const std::array<int, 20> extra_length_addend = {
      11, 13, 15, 17, 19, 23, 27, 31, 35, 43,
      51, 59, 67, 83, 99, 115, 131, 163, 195, 227};
  static const std::array<int, 26> extra_dist_addend = {
      4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128,
      192, 256, 384, 512, 768, 1024, 1536, 2048,
      3072, 4096, 6144, 8192, 12288, 16384, 24576};
  inflate::Decoder literals_dec, distance_dec;

  if (!fixedcode) {
    std::tie(literals_dec, distance_dec) = read_deflate_header(in);
  } else {
    literals_dec = build_decoder(inflate::fixedranges);
    distance_dec;
  }

  while (true) {
    inflate::Symbol symbol = literals_dec.read_out(in);
    if (symbol < 256) {
      teeout << static_cast<char>(symbol);
    } else if (symbol == 256) {  // stop symbol is 256
      break;
    } else if (symbol < 286) {  // backpointer (3.2.5):
      int length, distance;
      if (symbol < 265) {       // length of match
        length = symbol - 254;  // 257-264 -> 3-10
      } else if (symbol < 285) {
        int extra_bits = in.read((symbol - 261) / 4);
        length         = extra_bits  // 265-284 -> 11-257
                 + extra_length_addend[symbol - 265];
      } else {  // symbol == 285
        length = 258;
      }

      // read the distance code
      if (distance_dec.empty()) {
        distance = in.read(5);
      } else {
        distance = distance_dec.read_out(in);
      }

      if (distance < 30) {
        if (distance > 3) {  // 4-29 -> 4-32767
          int extra_dist = in.read((distance - 2) / 2);
          distance       = extra_dist + extra_dist_addend[distance - 4];
        }
        ++distance;  // 0-32767 -> 1-32768

        std::cout << buf.readfrom(length, distance);

      } else {
        throw std::invalid_argument(
            "Error decoding block: Invalid distance symbol");
      }
    } else {
      throw std::invalid_argument(
          "Error decoding block: Invalid literal symbol");
    }
  }

  return buf;
}


void inflate::gunzip(std::string fn, std::ostream& output /*=std::cout*/) {
  inflate::gzip_file file;
  std::ifstream in;

  // may throw the following
  in.exceptions(std::ios::badbit | std::ios::failbit);

  in.open(fn, std::ios::in | std::ios::binary);
  in.read((char*) &file.header, sizeof(gzip_header));
  // 1f8b signifies a gzip file
  if (file.header.id[0] != 0x1f || file.header.id[1] != 0x8b) {
    throw std::invalid_argument("Not in gzip format");
  }
  if (file.header.compression_method != 8) {
    throw std::invalid_argument("Compression Method not 8");
  }
  if (file.header.flags & flag::extra) {
    // TODO spec seems to indicate that this is little-endian;
    // htons for big-endian machines?
    in.read(reinterpret_cast<char*>(file.xlen), 2);
    in.read((char*) file.extra, file.xlen);
  }
  if (file.header.flags & flag::fname) {
    std::getline(in, file.fname, '\0');
  }
  if (file.header.flags & flag::comment) {
    std::getline(in, file.fcomment, '\0');
  }
  if (file.header.flags & flag::hcrc) {
    in.read(reinterpret_cast<char*>(file.crc16), 2);
  }

  // no longer throws exceptions
  in.exceptions(std::ios::goodbit);

  int last_block;
  unsigned char block_format;

  ifbstream bin(in);
  ringbuffer buf(inflate::max_buffer_size);
  do {
    last_block   = bin.next();
    block_format = bin.read(2);
    switch (block_format) {
      case 0x00:
        // TODO
        std::cerr << "Uncompressed block type not supported"
                  << std::endl;
        throw std::logic_error(
            "Uncompressed block type not implemented");
        break;
      case 0x01:
        inflate_block(bin, buf, output, true);  // fixedcode = true
        break;
      case 0x02:
        inflate_block(bin, buf, output);
        break;
      default:
        std::cerr << "Unsupported block type "
                  << int(block_format) << std::endl;
        throw std::invalid_argument("Invalid block type");
    }
  } while (!last_block);
}
