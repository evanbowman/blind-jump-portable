#pragma once

#include <stdint.h>


using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;


template <typename T>
struct Vec2 {
    T x;
    T y;
};
