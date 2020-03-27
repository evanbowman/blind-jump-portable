#pragma once

#include "transientEffect.hpp"


class Explosion
    : public TransientEffect<TextureMap::explosion, 6, milliseconds(55)> {
public:
    Explosion(const Vec2<Float>& position);
};
