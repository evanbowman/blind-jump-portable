#pragma once

#include "graphics/animation.hpp"
#include "orbshot.hpp"


class Player;


class WandererSmallLaser : public OrbShot {
public:
    WandererSmallLaser(const Vec2<Float>& position,
                       const Vec2<Float>& target,
                       Float speed);

    void update(Platform& pf, Game& game, Microseconds dt);

private:
    Microseconds flicker_timer_;
};
