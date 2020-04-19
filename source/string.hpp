#pragma once


#include "memory/buffer.hpp"
#include "number/numeric.hpp"


// For embedded systems, we cannot pull in all of libc, and we do not want to
// use the standard implementations of the libc functions, because we do not
// know how large or complex the implementations of those functions will be, in
// terms of code footprint. Necessary libc string functions will be provided in

// this header, along with other non-standard string functions. The game itself
// does not need complex logic for manipulating strings--instead, the overlay
// code has plenty of classes for displaying formatted text.


inline u32 str_len(const char* str)
{
    const char* s;

    for (s = str; *s; ++s)
        ;
    return (s - str);
}


inline void str_reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;

    while (start < end) {
        std::swap(*(str + start), *(str + end));
        start++;
        end--;
    }
}


inline int strcmp(const char* p1, const char* p2)
{
    const unsigned char* s1 = (const unsigned char*)p1;
    const unsigned char* s2 = (const unsigned char*)p2;

    unsigned char c1, c2;

    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;

        if (c1 == '\0') {
            return c1 - c2;
        }

    } while (c1 == c2);

    return c1 - c2;
}


// A not great, but satisfactory implementation of a string class.
template <u32 Capacity> class StringBuffer {
public:
    StringBuffer()
    {
        mem_.push_back('\0');
    }

    void push_back(char c)
    {
        if (not mem_.full()) {
            mem_[mem_.size() - 1] = c;
            mem_.push_back('\0');
        }
    }

    bool operator==(const char* str)
    {
        if (str_len(str) not_eq str_len(this->c_str())) {
            return false;
        }

        return strcmp(str, this->c_str()) == 0;
    }

    bool full() const
    {
        return mem_.full();
    }

    void clear()
    {
        mem_.clear();
        mem_.push_back('\0');
    }

    const char* c_str() const
    {
        return mem_.data();
    }

private:
    Buffer<char, Capacity + 1> mem_;
};
