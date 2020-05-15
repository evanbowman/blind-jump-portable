#pragma once

#include "number/numeric.hpp"
#include "function.hpp"
#include "bitvector.hpp"
#include "string.hpp"


namespace utf8 {

    using Codepoint = u32;


    namespace detail {
        inline bool is_little_endian()
        {
            short int number = 0x1;
            char *numPtr = (char*)&number;
            return (numPtr[0] == 1);
        }
    }

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
                std::array<char, 4> cp;
                if (detail::is_little_endian()) {
                    cp = {0, 0, data[index + 1], data[index]};
                } else {
                    cp = {data[index], data[index + 1], 0, 0};
                }
                index += 2;
                callback(*reinterpret_cast<Codepoint*>(cp.data()));
            } else if (parsed[7] == 1 and parsed[6] == 1 and parsed[5] == 1 and
                       parsed[4] == 0) {
                std::array<char, 4> cp;
                if (detail::is_little_endian()) {
                    cp = {0, data[index + 2], data[index + 1], data[index]};
                } else {
                    cp = {data[index], data[index + 1], data[index + 2]};
                }
                index += 3;
                callback(*reinterpret_cast<Codepoint*>(cp.data()));
            } else if (parsed[7] == 1 and parsed[6] == 1 and parsed[5] == 1 and
                       parsed[4] == 1 and parsed[3] == 0) {
                std::array<char, 4> cp;
                if (detail::is_little_endian()) {
                    cp = {data[index + 3],
                          data[index + 2],
                          data[index + 1],
                          data[index]};
                } else {
                    cp = {data[index],
                          data[index + 1],
                          data[index + 2],
                          data[index + 3]};
                }
                callback(*reinterpret_cast<Codepoint*>(cp.data()));
                index += 4;
            } else {
                return false;
            }
        }
        return true;
    }

    inline size_t len(const char* data)
    {
        size_t ret = 0;
        scan([&ret](const Codepoint&) { ++ret; }, data, str_len(data));
        return ret;
    }
}
