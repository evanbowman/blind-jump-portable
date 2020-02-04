#pragma once

// NOTE: The snake enemy takes up a lot of resources, we can only realistically
// spawn one per level.



#include "entity/entity.hpp"
#include "tileMap.hpp"
#include "collision.hpp"


class Player;
class Laser;



class SnakeNode : public Entity {
public:
    SnakeNode(SnakeNode* parent);

    SnakeNode* parent() const;

    void update();

    const Vec2<TIdx>& tile_coord() const;

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

private:
    SnakeNode* parent_;
    Vec2<TIdx> tile_coord_;
    HitBox hitbox_;
};



class SnakeHead : public SnakeNode {
public:
    SnakeHead(const Vec2<Float>& pos, Game& game);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void on_collision(Platform& pf, Game& game, Player&) {}
    void on_collision(Platform& pf, Game& game, Laser&) {}


    const Sprite& get_shadow() const
    {
        return shadow_;
    }

private:

    Sprite shadow_;

    enum class Dir { up, down, left, right } dir_;
};



class SnakeBody : public SnakeNode {
public:

    SnakeBody(const Vec2<Float>& pos, SnakeNode* parent, Game&, u8 remaining);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void on_collision(Platform& pf, Game& game, Player&) {}
    void on_collision(Platform& pf, Game& game, Laser&) {}

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

private:

    Sprite shadow_;

    Vec2<TIdx> next_coord_;
};



class SnakeTail : public SnakeBody {
public:

    SnakeTail(const Vec2<Float>& pos, SnakeNode* parent, Game& game);
};
