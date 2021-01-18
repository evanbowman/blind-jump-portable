#pragma once

#include "blind_jump/entity/entity.hpp"
#include <optional>


class Particle : public Entity {
public:
    Particle(const Vec2<Float>& position,
             std::optional<ColorConstant> color,
             Float drift_speed = 0.0000338f,
             bool x_wave_effect = false,
             Microseconds duration = seconds(1),
             Microseconds offset = 0,
             std::optional<Float> angle = {},
             const Vec2<Float>& reference_speed = {});

    void update(Platform& pfrm, Game& game, Microseconds dt);

private:
    Microseconds timer_;
    const Microseconds duration_;
    Vec2<Float> step_vector_;
    bool x_wave_effect_;
};
