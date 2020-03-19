#pragma once


#include "collision.hpp"
#include "entity/entity.hpp"
#include "graphics/animation.hpp"


class Player;
class Laser;


class Turret : public Entity {
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

    void on_collision(Platform&, Game&, Laser&);

private:
    Sprite shadow_;
    Animation<TextureMap::turret, 5, milliseconds(50)> animation_;
    enum class State { sleep, closed, opening, open, closing } state_;
    HitBox hitbox_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Microseconds timer_;
};
