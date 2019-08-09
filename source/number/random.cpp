#include "random.hpp"


static int seed;


int& random_seed()
{
    return seed;
}


int random_value()
{
    seed = 1664525 * seed + 1013904223;
    return (seed >> 16) & 0x7FFF;
}
