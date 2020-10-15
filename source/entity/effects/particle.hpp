#pragma once

#include "entity/entity.hpp"


class Particle : public Entity {
public:
    Particle(const Vec2<Float>& position,
             ColorConstant color,
             Float drift_speed = 0.0000338f,
             Microseconds duration = seconds(1),
             Microseconds offset = 0,
             std::optional<Float> angle = {},
             const Vec2<Float>& reference_speed = {});

    void update(Platform& pfrm, Game& game, Microseconds dt);

private:
    Microseconds timer_;
    const Microseconds duration_;
    Vec2<Float> step_vector_;
};
