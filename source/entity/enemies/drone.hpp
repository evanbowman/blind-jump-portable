#pragma once


#include "collision.hpp"
#include "entity/entity.hpp"


class Player;
class Laser;


class Drone : public Entity {
public:
    Drone(const Vec2<Float>& pos);

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

    void update(Platform&, Game&, Microseconds);

private:
    enum class State { sleep, inactive, move, idle } state_;

    Microseconds timer_;

    Vec2<Float> step_vector_;

    Sprite shadow_;
    HitBox hitbox_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
