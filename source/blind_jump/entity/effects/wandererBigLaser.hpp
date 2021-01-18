#pragma once

#include "graphics/animation.hpp"
#include "projectile.hpp"


class Player;


class WandererBigLaser : public Projectile {
public:
    WandererBigLaser(const Vec2<Float>& position,
                     const Vec2<Float>& target,
                     Float speed);

    void update(Platform& pf, Game& game, Microseconds dt);

    void on_collision(Platform& pf, Game&, Player&);

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

private:
    Microseconds timer_;
    Microseconds flicker_timer_;
    HitBox hitbox_;
    Animation<48, 2, milliseconds(20)> animation_;
};
