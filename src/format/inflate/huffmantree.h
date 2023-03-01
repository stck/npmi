#ifndef HUFFMANTREE_H
#define HUFFMANTREE_H

#include "inflate.h"
#include "ifbstream.h"

namespace inflate {
    typedef int Code;
    typedef int Symbol;

    class huffmantree;
    struct huffmannode;
}


class inflate::huffmantree : public inflate::Decodertype {
public:
    huffmantree() : root(nullptr) {}
    
    void insert(int codelength, Code, Symbol);
    inflate::Symbol read_out(ifbstream& in) const;
    inline bool empty() const noexcept
        {return root == nullptr;}
    std::string str();

private:
    inflate::huffmannode* root;
};


struct inflate::huffmannode {
    Symbol symbol;  // -1 indicates inner node
    inflate::huffmannode* zero;
    inflate::huffmannode* one;
};

#endif

