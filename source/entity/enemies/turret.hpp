#pragma once


#include "animation.hpp"
#include "collision.hpp"
#include "entity/entity.hpp"


class Player;


class Turret : public Entity<Turret, 6> {
public:
    Turret(const Vec2<Float>& pos);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

    void on_collision(Platform&, Game&, Player&)
    {
    }

private:
    Sprite shadow_;
    Animation<TextureMap::turret, 5, Microseconds(50000)> animation_;
    enum class State { closed, opening, open, closing } state_;
    HitBox hitbox_;
};
