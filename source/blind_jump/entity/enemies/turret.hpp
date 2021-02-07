#pragma once


#include "blind_jump/entity/entity.hpp"
#include "blind_jump/network_event.hpp"
#include "collision.hpp"
#include "enemy.hpp"
#include "graphics/animation.hpp"


class LaserExplosion;
class AlliedOrbShot;
class Player;
class Laser;


class Turret : public Enemy {
public:
    Turret(const Vec2<Float>& pos);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    using Enemy::on_collision;

    void on_collision(Platform&, Game&, LaserExplosion&);
    void on_collision(Platform&, Game&, AlliedOrbShot&);
    void on_collision(Platform&, Game&, Laser&);

    void on_death(Platform&, Game&);

    void sync(const net_event::EnemyStateSync& state, Game& game);

private:
    void injured(Platform&, Game&, Health amount);

    Animation<TextureMap::turret, 5, milliseconds(50)> animation_;
    enum class State { sleep, closed, opening, open1, open2, closing } state_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Microseconds timer_;
};
