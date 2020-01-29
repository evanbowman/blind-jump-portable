#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"


class Projectile : public Entity<Projectile, 10> {
public:

    Projectile(const Vec2<Float>& position, Angle angle, Float speed) :
        speed_(speed)
    {
        this->set_position(position);

        set_angle(angle);
    }

    void update(Platform&, Game&, Microseconds dt)
    {
        this->position_ = this->position_ + (speed_ * dt * direction_);
    }

    void set_angle(Angle angle)
    {
        angle_ = angle;

        direction_ = {
            Float(cosine(angle)) / INT16_MAX,
            Float(sine(angle)) / INT16_MAX
        };
    }

    Angle get_angle() const
    {
        return angle_;
    }

private:
    Angle angle_;
    Float speed_;

    Vec2<Float> direction_;
};
