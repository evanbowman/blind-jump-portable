#pragma once

#include "number/numeric.hpp"


class Color {
public:
    Color(u8 r, u8 g, u8 b) : r_(r), g_(g), b_(b)
    {
    }

    inline u16 bgr_hex_555() const
    {
        return (r_) + ((g_) << 5) + ((b_) << 10);
    }

    static Color from_bgr_hex_555(u16 val)
    {
        return {
            u8(0x1F & val), u8((0x3E0 & val) >> 5), u8((0x7C00 & val) >> 10)};
    }

    inline Color invert() const
    {
        constexpr u8 max{31};
        return {u8(max - r_), u8(max - g_), u8(max - b_)};
    }

    inline Color grayscale() const
    {
        const u8 val = 0.3f * r_ + 0.59f * g_ + 0.11f * b_;
        return {val, val, val};
    }

    u8 r_;
    u8 g_;
    u8 b_;
};
