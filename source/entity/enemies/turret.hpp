#pragma once


#include "collision.hpp"
#include "enemy.hpp"
#include "entity/entity.hpp"
#include "graphics/animation.hpp"


class LaserExplosion;
class Player;
class Laser;


class Turret : public Enemy {
public:
    Turret(const Vec2<Float>& pos);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void on_collision(Platform&, Game&, LaserExplosion&);
    void on_collision(Platform&, Game&, Laser&);
    void on_collision(Platform&, Game&, Player&)
    {
    }

    void on_death(Platform&, Game&);

private:

    void injured(Platform&, Game&, Health amount);

    Animation<TextureMap::turret, 5, milliseconds(50)> animation_;
    enum class State { sleep, closed, opening, open, closing } state_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Microseconds timer_;
};
