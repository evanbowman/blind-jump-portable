#pragma once


#include "numeric.hpp"


namespace rng {

using Value = s32;
using Generator = Value;


extern Generator critical_state;

// NOTE: use the utility state whenever you need a random value, and you don't
// care whether the value is synchronized across multiplayer games.
extern Generator utility_state;


Value get(Generator& gen);


template <Value N> Value choice(Generator& gen)
{
    return get(gen) % N;
}


inline Value choice(Value n, Generator& gen)
{
    return fast_mod(get(gen), n);
}


template <u32 offset> Float sample(Float n, Generator& gen)
{
    if (choice<2>(gen)) {
        return n + Float(choice<offset>(gen));

    } else {
        return n - Float(choice<offset>(gen));
    }
}


template <u32 offset>
Vec2<Float> sample(const Vec2<Float>& position, Generator& gen)
{
    auto result = position;

    result.x = sample<offset>(result.x, gen);
    result.y = sample<offset>(result.y, gen);

    return result;
}

} // namespace rng
