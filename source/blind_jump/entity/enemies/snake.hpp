#pragma once

// NOTE: The snake enemy takes up a lot of resources, we can only realistically
// spawn one per level.


#include "blind_jump/entity/entity.hpp"
#include "blind_jump/network_event.hpp"
#include "collision.hpp"
#include "enemy.hpp"
#include "tileMap.hpp"


class LaserExplosion;
class AlliedOrbShot;
class Player;
class Laser;


class SnakeNode : public Enemy {
public:
    SnakeNode(SnakeNode* parent);

    SnakeNode* parent() const;

    void update(Platform& pfrm, Game& game, Microseconds dt);

    const Vec2<TIdx>& tile_coord() const;

    friend class SnakeTail;

    void sync(const net_event::EnemyStateSync&, Game&)
    {
        // NOTE: We have not implemented multiplayer synchronization for the
        // snake enemy. If we did, we would need to re-position all of the tail
        // segments, and make it look smooth, which is complicated, so lets just
        // let this enemy be out of sync.
    }

protected:
    void destroy();

    enum class State { sleep, active } state_ = State::sleep;

private:
    SnakeNode* parent_;
    Vec2<TIdx> tile_coord_;
    Microseconds destruct_timer_;
};


class SnakeHead : public SnakeNode {
public:
    SnakeHead(const Vec2<Float>& pos, Game& game);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    using Enemy::on_collision;

    void on_collision(Platform&, Game&, Laser&)
    {
    }

    void on_collision(Platform&, Game&, LaserExplosion&)
    {
    }

    void on_collision(Platform&, Game&, AlliedOrbShot&)
    {
    }

private:
    enum class Dir { up, down, left, right } dir_;
};


class SnakeBody : public SnakeNode {
public:
    SnakeBody(const Vec2<Float>& pos, SnakeNode* parent, Game&, u8 remaining);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    using Enemy::on_collision;

    void on_collision(Platform&, Game&, Laser&)
    {
    }

    void on_collision(Platform&, Game&, LaserExplosion&)
    {
    }

    void on_collision(Platform&, Game&, AlliedOrbShot&)
    {
    }

protected:
    Vec2<TIdx> next_coord_;
};


class SnakeTail : public SnakeBody {
public:
    SnakeTail(const Vec2<Float>& pos, SnakeNode* parent, Game& game);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    using Enemy::on_collision;

    void on_collision(Platform& pf, Game& game, Laser&);
    void on_collision(Platform&, Game&, LaserExplosion&);
    void on_collision(Platform&, Game&, AlliedOrbShot&);

    void on_death(Platform&, Game&);

private:
    Microseconds sleep_timer_;
    Microseconds drop_timer_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
