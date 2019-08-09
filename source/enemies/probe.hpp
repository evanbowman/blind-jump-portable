#pragma once

#include "entity.hpp"
#include "collision.hpp"
#include "platform.hpp"


class Probe : public Entity<Probe, 20> {
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
