#pragma once

#include "numeric.hpp"


class Color {
public:
    Color(u8 r, u8 g, u8 b) : r_(r), g_(g), b_(b)
    {
    }

    inline u16 hex_555() const
    {
        return (r_) + ((g_) << 5) + ((b_) << 10);
    }

private:
    u8 r_;
    u8 g_;
    u8 b_;
};


