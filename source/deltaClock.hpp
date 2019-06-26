#pragma once

#include "numeric.hpp"


using Microseconds = u32;


class DeltaClock {
public:

    DeltaClock();

    Microseconds reset();

private:
    void* impl_;
};
