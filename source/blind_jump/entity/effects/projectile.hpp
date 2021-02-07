#pragma once

#include "blind_jump/entity/entity.hpp"
#include "collision.hpp"


class Projectile : public Entity {
public:
    Projectile(const Vec2<Float>& position,
               const Vec2<Float>& target,
               Float speed)
    {
        this->set_position(position);
        sprite_.set_position(position);

        step_vector_ = direction(position, target) * speed;
    }

    void update(Platform&, Game&, Microseconds dt)
    {
        this->position_ = this->position_ + Float(dt) * step_vector_;
        sprite_.set_position(position_);
    }

    void rotate(Angle angle)
    {
        step_vector_ = ::rotate(step_vector_, angle);
    }

private:
    Vec2<Float> step_vector_;
};
