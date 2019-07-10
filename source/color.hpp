#pragma once

#include "numeric.hpp"


enum class ColorConstant {
    electric_blue,
    ruby,
};


struct ColorMix {
    ColorConstant color_;
    Float amount_;
};
