#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"


class Player;


class Enemy : public Entity {
public:
    Enemy(Entity::Health health,
          const Vec2<Float>& position,
          const HitBox::Dimension& dimension) :
        Entity(health),
        hitbox_{&position_, dimension}
    {
        position_ = position;
    }

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

protected:
    Sprite shadow_;
    HitBox hitbox_;
};
