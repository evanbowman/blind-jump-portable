#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"


class Projectile : public Entity {
public:

    Projectile(const Vec2<Float>& position, Angle angle, Float speed) :
        speed_(speed)
    {
        this->set_position(position);

        set_angle(angle);

        sprite_.set_texture_index(TextureMap::coin);
        sprite_.set_size(Sprite::Size::w16_h32);
    }

    void update(Platform&, Game&, Microseconds dt)
    {
        this->position_ = this->position_ + (speed_ * dt * direction_);
        sprite_.set_position(position_);
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
