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
    return fast_mod(random_value(), n); // * n >> 15;
}


template <u32 offset> static Vec2<Float> sample(const Vec2<Float>& position)
{
    auto result = position;

    if (random_choice<2>()) {
        result.x += Float(random_choice<offset>());
    } else {
        result.x -= Float(random_choice<offset>());
    }

    if (random_choice<2>()) {
        result.y += Float(random_choice<offset>());
    } else {
        result.y -= Float(random_choice<offset>());
    }

    return result;
}
