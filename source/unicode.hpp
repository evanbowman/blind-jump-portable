#pragma once

#include "bitvector.hpp"
#include "function.hpp"
#include "number/numeric.hpp"
#include "string.hpp"
#include <optional>


namespace utf8 {

using Codepoint = u32;


inline bool scan(Function<32, void(const Codepoint& cp)> callback,
                 const char* data,
                 size_t len)
{
    size_t index = 0;
    while (index < len) {
        const Bitvector<8> parsed(data[index]);
        if (parsed[7] == 0) {
            callback(data[index]);
            index += 1;
        } else if (parsed[7] == 1 and parsed[6] == 1 and parsed[5] == 0) {
            callback(data[index] | (data[index + 1] << 8));
            index += 2;
        } else if (parsed[7] == 1 and parsed[6] == 1 and parsed[5] == 1 and
                   parsed[4] == 0) {
            callback(data[index] | (data[index + 1] << 8) |
                     (data[index + 2] << 16));
            index += 3;
        } else if (parsed[7] == 1 and parsed[6] == 1 and parsed[5] == 1 and
                   parsed[4] == 1 and parsed[3] == 0) {
            callback(data[index] | (data[index + 1] << 8) |
                     (data[index + 2] << 16) | (data[index + 3] << 24));
            index += 4;
        } else {
            return false;
        }
    }
    return true;
}


// This returns the first utf8 Codepoint in a string, meant mostly as a utility
// for creating codepoint literals from strings. Unfortunately, C++ doesn't
// offer unicode character literals... I guess I could specify them in hex, but
// that's no fun (and not so easy for other people to read).
inline Codepoint getc(const char* data)
{
    std::optional<Codepoint> front;
    scan(
        [&front](const Codepoint& cp) {
            if (not front)
                front = cp;
        },
        data,
        str_len(data));

    if (front) {
        return *front;
    } else {
        return '\0';
    }
}


inline size_t len(const char* data)
{
    size_t ret = 0;
    scan([&ret](const Codepoint&) { ++ret; }, data, str_len(data));
    return ret;
}
} // namespace utf8
