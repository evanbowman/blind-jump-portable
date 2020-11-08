#pragma once

#include "entity/enemies/enemy.hpp"
#include "localeString.hpp"


class LaserExplosion;
class AlliedOrbShot;
class Player;
class Laser;


class InfestedCore : public Enemy {
public:

    InfestedCore(const Vec2<Float>& position);

    inline LocaleString defeated_text() const
    {
        return LocaleString::boss2_defeated;
    }

    bool is_allied() const { return false; }

    void update(Platform& pf, Game& game, Microseconds dt);

    void on_collision(Platform&, Game&, LaserExplosion&) {}
    void on_collision(Platform&, Game&, AlliedOrbShot&) {}
    void on_collision(Platform&, Game&, Player&) {}
    void on_collision(Platform&, Game&, Laser&) {}

};
