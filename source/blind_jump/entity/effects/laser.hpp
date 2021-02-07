#pragma once

#include "blind_jump/entity/entity.hpp"
#include "collision.hpp"


class Turret;
class Dasher;
class SnakeHead;
class SnakeBody;
class Drone;


class Laser : public Entity {
public:
    enum class Mode { normal, explosive };

    Laser(const Vec2<Float>& position, Cardinal dir, Mode mode);

    void update(Platform& pf, Game& game, Microseconds dt);

    template <typename T> void on_collision(Platform& pfrm, Game& game, T&)
    {
        this->kill();
        handle_collision(pfrm, game);
    }

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

private:
    void handle_collision(Platform&, Game&);

    Cardinal dir_;
    Microseconds timer_;
    HitBox hitbox_;
    Mode mode_;
};


class PeerLaser : public Laser {
public:
    using Laser::Laser;
};
