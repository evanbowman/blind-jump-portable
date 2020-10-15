#include "random.hpp"


rng::LinearGenerator rng::critical_state;
rng::LinearGenerator rng::utility_state;


rng::Value rng::get(LinearGenerator& gen)
{
    gen = 1664525 * gen + 1013904223;
    return (gen >> 16) & 0x7FFF;
}
