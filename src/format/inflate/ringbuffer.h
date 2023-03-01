#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <sstream>
#include <string>

class ringbuffer : public std::stringstream {
public:
    ringbuffer(int size)
        : max_buffer_size(size) {}
    std::string readfrom(int length, int distance);

private:
    int max_buffer_size;
};

#endif
