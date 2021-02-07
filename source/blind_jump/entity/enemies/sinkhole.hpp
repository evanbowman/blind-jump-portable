#pragma once

#include "blind_jump/network_event.hpp"
#include "enemy.hpp"


class LaserExplosion;
class AlliedOrbShot;


class Sinkhole : public Enemy {
public:
    Sinkhole(const Vec2<Float>& pos);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    using Enemy::on_collision;

    void on_collision(Platform&, Game&, LaserExplosion&)
    {
    }

    void on_collision(Platform&, Game&, AlliedOrbShot&)
    {
    }

    void on_death(Platform& pfr, Game& game);


    void sync(const net_event::EnemyStateSync&, Game&)
    {
        // NOTE: Unsynchronized in multiplayer. This enemy neither deals damage,
        // nor takes damage. Repositioning would also be inappropriate, as we do
        // not synchronize the snake enemy.
    }

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
