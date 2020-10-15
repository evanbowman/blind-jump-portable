#pragma once


#include "memory/buffer.hpp"
#include "numeric.hpp"


namespace rng {

using Value = s32;
using LinearGenerator = Value;

// NOTE: This state should be used for level generation, and for the internal
// state machines for enemies. This state needs to be tightly synchronized
// between multiplayer peers, so do not use this generator for visual effects,
// game state changes only!
extern LinearGenerator critical_state;

// NOTE: use the utility state whenever you need a random value, and you don't
// care whether the value is synchronized across multiplayer games.
extern LinearGenerator utility_state;


Value get(LinearGenerator& gen);


template <Value N> Value choice(LinearGenerator& gen)
{
    return get(gen) % N;
}


inline Value choice(Value n, LinearGenerator& gen)
{
    return get(gen) % n;
}


template <u32 offset> Float sample(Float n, LinearGenerator& gen)
{
    if (choice<2>(gen)) {
        return n + Float(choice<offset>(gen));

    } else {
        return n - Float(choice<offset>(gen));
    }
}


template <u32 offset>
Vec2<Float> sample(const Vec2<Float>& position, LinearGenerator& gen)
{
    auto result = position;

    result.x = sample<offset>(result.x, gen);
    result.y = sample<offset>(result.y, gen);

    return result;
}


template <typename T, u32 size>
void shuffle(Buffer<T, size>& buffer, LinearGenerator& gen)
{
    int i;
    const int n = buffer.size();
    for (i = n - 1; i > 0; --i) {
        std::swap(buffer[i], buffer[get(gen) % (i + 1)]);
    }
}


} // namespace rng
