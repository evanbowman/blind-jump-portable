#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"


class Player;
class OrbShot;


class Enemy : public Entity {
public:
    Enemy(Entity::Health health,
          const Vec2<Float>& position,
          const HitBox::Dimension& dimension)
        : Entity(health), hitbox_{&position_, dimension}, is_allied_(false)
    {
        position_ = position;
    }

    void update(Platform&, Game&, Microseconds)
    {
        if (is_allied_ and sprite_.get_mix().amount_ == 0) {
            sprite_.set_mix({ColorConstant::green, 230});
        }
    }

    static constexpr bool has_shadow = true;

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

    const Entity& get_target(Game& game);

    OrbShot* shoot(Platform&,
                   Game&,
                   const Vec2<Float>& position,
                   const Vec2<Float>& target,
                   Float speed,
                   Microseconds duration = seconds(2));

    bool is_allied() const
    {
        return is_allied_;
    }

    void make_allied(bool allied)
    {
        is_allied_ = allied;
    }

protected:
    void debit_health(Platform& pfrm, Health amount = 1);

    Sprite shadow_;
    HitBox hitbox_;
    bool is_allied_;
};
