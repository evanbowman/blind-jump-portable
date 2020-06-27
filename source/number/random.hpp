#pragma once


#include "numeric.hpp"


namespace rng {

using Value = int;
using Generator = Value;


extern Generator global_state;


Value get(Generator& gen = global_state);


template <Value N> Value choice(Generator& gen = global_state)
{
    return get(gen) % N;
}


inline Value choice(Value n, Generator& gen = global_state)
{
    return fast_mod(get(gen), n);
}


template <u32 offset> Float sample(Float n, Generator& gen = global_state)
{
    if (choice<2>(gen)) {
        return n + Float(choice<offset>(gen));

    } else {
        return n - Float(choice<offset>(gen));
    }
}


template <u32 offset>
Vec2<Float> sample(const Vec2<Float>& position, Generator& gen = global_state)
{
    auto result = position;

    result.x = sample<offset>(result.x, gen);
    result.y = sample<offset>(result.y, gen);

    return result;
}

} // namespace rng
