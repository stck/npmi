#include "ringbuffer.h"

#include <iostream>
#include <fstream>

#include "teestream.h"

std::string ringbuffer::readfrom(int length, int distance) {
    std::stringstream out;
    teestream tee(*this, out);

    this->flush();
    int backi = (int)this->tellp() - distance;
#ifdef DEBUG_DUMP_CODES
    std::cout << "from " << backi << ":";
#endif
    // access buf as a ringbuffer
    if (backi < 0) {
        this->seekg(0, std::ios_base::end);
        int buffer_size = this->tellg();
        if (buffer_size < max_buffer_size) {
            // buffer has not been filled
            throw std::invalid_argument("Backpointer exceeds buffer size");
        }
        backi += buffer_size;
#ifdef DEBUG_DUMP_CODES
        std::cout << "aka " << backi << ':';
        if(length == 7 && distance == 16928) {
            std::cerr << this->tellp() << std::endl;
            int temp = this->tellg();
            std::ofstream log("buf.log");
            this->seekg(0);
            log << this->rdbuf();
            this->seekg(temp);
        }
#endif
        // reading across the link?
        if ((buffer_size - backi) <= length) {
            // read to the link
            this->seekg(backi);
            tee << this->rdbuf();
            // continue past the link
            length -= buffer_size - backi;
            backi = 0;
        }
    }

    this->seekg(backi);
    for(int i=0; i<length; i++) {
        tee << (char)this->get();
    }

    // set link if past limit
    if (this->tellp() >= this->max_buffer_size) {
        this->max_buffer_size = this->tellp();
        this->seekp(0);
    }

    return out.str();
}
