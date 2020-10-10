#pragma once

#include "entity/entity.hpp"


class Particle : public Entity {
public:
    Particle(const Vec2<Float>& position,
             ColorConstant color,
             Float drift_speed = 0.0000338f);

    void update(Platform& pfrm, Game& game, Microseconds dt);

private:
    Microseconds timer_;
    Vec2<Float> step_vector_;
};
