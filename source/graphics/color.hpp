#pragma once

#include "number/numeric.hpp"


// Color representation is often platform specific, e.g., OpenGL uses RGBA,
// Nintendo GameBoy uses 15 bit BGR (5 bits per channel). For the sake of
// portability, the colors are described only with an enum, and the
// implementations of the Platform header are responsible for using the correct
// values.
enum class ColorConstant {
    null,
    // clang-format off
    electric_blue     = 0x00FFFF,
    turquoise_blue    = 0x00FFDD,
    cerulean_blue     = 0x66E0FF,
    picton_blue       = 0x4DACFF,
    maya_blue         = 0x4FBCFF,
    steel_blue        = 0x345680,
    spanish_crimson   = 0xE81858,
    aerospace_orange  = 0xFD5200,
    safety_orange     = 0xFC7500,
    rich_black        = 0x000010,
    stil_de_grain     = 0xF9DC5C,
    silver_white      = 0xF4F4F8,
    aged_paper        = 0xDEC397,
    green             = 0x27C8AF,
    med_blue_gray     = 0x7674A9,
    indigo_tint       = 0x255F85,
    // clang-format on
};


inline ColorConstant custom_color(int hex)
{
    return static_cast<ColorConstant>(hex);
}


inline ColorConstant custom_color(u8 r, u8 g, u8 b)
{
    return static_cast<ColorConstant>((r << 16) | (g << 8) | b);
}


struct ColorMix {
    ColorConstant color_ = ColorConstant::null;
    u8 amount_ = 0;
};
