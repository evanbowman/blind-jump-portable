#pragma once


#include "numeric.hpp"


int& random_seed();


int random_value();


template <u32 N> inline int random_choice()
{
    return random_value() % N; // * N >> 15;
}

inline int random_choice(u32 n)
{
    return random_value() % n; // * n >> 15;
}
