#include "huffmantree.h"
#include "ifbstream.h"

void inflate::huffmantree::insert(int codelength,
    inflate::Code code, inflate::Symbol symbol) {
  if (root == nullptr) {
    root = new inflate::huffmannode({-1, nullptr, nullptr});
  }
  inflate::huffmannode* curr = root;
  // read code bit by bit
  for (int i = codelength - 1; i >= 0; i--) {
    if ((code >> i) & 0x01) {  // 1
      if (curr->one == nullptr) {
        curr->one = new inflate::huffmannode({-1, nullptr, nullptr});
      }
      curr = curr->one;
    } else {  // 0
      if (curr->zero == nullptr) {
        curr->zero = new inflate::huffmannode({-1, nullptr, nullptr});
      }
      curr = curr->zero;
    }
  }
  curr->symbol = symbol;
}

inflate::Symbol inflate::huffmantree::read_out(ifbstream& in) const {
  /*Decode a single symbol from stream in, using the huffman_tree*/
  inflate::huffmannode* curr = root;
  inflate::Symbol symbol     = curr->symbol;

  while (symbol < 0) {
    if (in.next()) {  // next bit is 1
      curr = curr->one;
    } else {  // next bit is 0
      curr = curr->zero;
    }

    if (curr == nullptr) {  // this leaf has code -1 probably
      throw std::invalid_argument("Malformed tree, unindexed code");
    }
    symbol = curr->symbol;
  }
  
  return symbol;
}

#include "binarytreenode.h"

std::string inflate::huffmantree::str() {
  return serialize(root);
}
