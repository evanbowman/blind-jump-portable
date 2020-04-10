#pragma once


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
