#pragma once

#include "projectile.hpp"


class Player;


class ConglomerateShot : public Projectile {
public:
    ConglomerateShot(const Vec2<Float>& position,
                     const Vec2<Float>& target,
                     Float speed,
                     Microseconds duration = seconds(1));


    void update(Platform& pf, Game& game, Microseconds dt);

    void on_collision(Platform& pf, Game&, Player&);

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

    void on_death(Platform&, Game&);

private:
    Microseconds timer_;
    Microseconds flicker_timer_;
    HitBox hitbox_;
};
