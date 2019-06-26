#include "deltaClock.hpp"


DeltaClock::DeltaClock() : impl_(nullptr)
{

}


Microseconds DeltaClock::reset()
{
    // Assumes 60 fps
    constexpr Microseconds fixed_step = 16667;
    return fixed_step;
}
