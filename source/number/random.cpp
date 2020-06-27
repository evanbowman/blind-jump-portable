#include "random.hpp"


rng::Generator rng::global_state;


rng::Value rng::get(Generator& gen)
{
    gen = 1664525 * gen + 1013904223;
    return (gen >> 16) & 0x7FFF;
}
