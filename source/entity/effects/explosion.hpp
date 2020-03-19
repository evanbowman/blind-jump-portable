#pragma once

#include "entity/entity.hpp"
#include "graphics/animation.hpp"


class Explosion : public Entity {
public:
    Explosion(const Vec2<Float>& position);

    void update(Platform& pfrm, Game& game, Microseconds dt);

private:
    Animation<TextureMap::explosion, 6, milliseconds(55)> animation_;
};
