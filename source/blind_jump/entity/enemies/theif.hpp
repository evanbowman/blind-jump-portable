#pragma once

#include "enemy.hpp"


class LaserExplosion;
class AlliedOrbShot;
class Player;
class Laser;


class Theif : public Enemy {
public:
    Theif(const Vec2<Float>& pos);

    void update(Platform&, Game&, Microseconds);

    using Enemy::on_collision;

    void on_collision(Platform&, Game&, LaserExplosion&);
    void on_collision(Platform&, Game&, AlliedOrbShot&);
    void on_collision(Platform&, Game&, Player&);
    void on_collision(Platform&, Game&, Laser&);

    void on_death(Platform&, Game&);

private:
    void injured(Platform&, Game&, Health amount);

    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Vec2<Float> step_vector_;
    Microseconds timer_;
    Microseconds shadow_check_timer_;
    int float_amount_;
    Microseconds float_timer_;
    u8 tries_;
    enum class State { inactive, sleep, idle, approach } state_;
};
