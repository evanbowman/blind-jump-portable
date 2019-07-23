#pragma once


#include "numeric.hpp"


struct SaveData {
    static constexpr u32 magic_val = 0xCA55E77E;

    using Identity = u64;
    using Magic = u32;

    Identity id_ = 0;
    Magic magic_ = magic_val;
    u32 seed_ = 42;
};
