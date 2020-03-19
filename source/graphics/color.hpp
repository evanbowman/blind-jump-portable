#pragma once

#include "number/numeric.hpp"


// Color representation is often platform specific, e.g., OpenGL uses RGBA,
// Nintendo GameBoy uses 15 bit BGR (5 bits per channel). For the sake of
// portability, the colors are described only with an enum, and the
// implementations of the Platform header are responsible for using the correct
// values.
enum class ColorConstant {
    null,
    electric_blue,    // #48F8F8
    spanish_crimson,  // #E81858
    aerospace_orange, // #FD5200
    rich_black,       // #000010
    chrome_yellow,    // #F0A202
    count
};


struct ColorMix {
    ColorConstant color_ = ColorConstant::null;
    u8 amount_ = 0;
};
