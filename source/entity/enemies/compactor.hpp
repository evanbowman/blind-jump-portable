#pragma once

#include "enemy.hpp"


class Laser;
class Player;
class PeerLaser;
class AlliedOrbShot;
class LaserExplosion;


class Compactor : public Enemy {
public:
    Compactor(const Vec2<Float>& position);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void on_collision(Platform& pfrm, Game& game, LaserExplosion&);
    void on_collision(Platform& pfrm, Game& game, AlliedOrbShot&);
    void on_collision(Platform& pfrm, Game& game, PeerLaser&);
    void on_collision(Platform& pfrm, Game& game, Player&);
    void on_collision(Platform& pfrm, Game& game, Laser&);

    void on_death(Platform& pfrm, Game& game);

    enum class State {
        sleep,
        await,
        peer_triggered_fall,
        fall,
        landing,
        pause,
        rush,
    };

    State state() const;

    void injured(Platform&, Game&, Health amount);

    void sync(const net_event::EnemyStateSync& state, Game&);

private:
    void set_movement_vector(const Vec2<Float>& target);

    State state_ = State::sleep;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Microseconds timer_;
    Vec2<Float> step_vector_;
};
