#pragma once

#include "enemy.hpp"


class LaserExplosion;
class AlliedOrbShot;


class Sinkhole : public Enemy {
public:
    Sinkhole(const Vec2<Float>& pos);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void on_collision(Platform&, Game&, LaserExplosion&)
    {
    }

    void on_collision(Platform&, Game&, AlliedOrbShot&)
    {
    }

    void on_death(Platform& pfr, Game& game);

private:
    void injured(Platform&, Game&, Health amount);

    enum class State {
        pause,
        rise,
        open,
    } state_;


    // FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Microseconds timer_;
    Microseconds anim_timer_;
};
