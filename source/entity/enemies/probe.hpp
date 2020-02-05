#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"
#include "platform/platform.hpp"


class Probe : public Entity {
public:
    Probe(const Vec2<Float>& position);

    void update(Platform&, Game&, Microseconds);

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

private:
    Sprite shadow_;
    HitBox hitbox_;
};
