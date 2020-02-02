#pragma once



#include "entity/entity.hpp"
#include "tileMap.hpp"



class SnakeNode : public Entity {
public:
    SnakeNode(SnakeNode* parent);

    SnakeNode* parent() const;

    void update();

    const Vec2<TIdx>& tile_coord() const;

private:
    SnakeNode* parent_;
    Vec2<TIdx> tile_coord_;
};



class SnakeHead : public SnakeNode {
public:
    SnakeHead(const Vec2<Float>& pos, Game& game);

    void update(Platform& pfrm, Game& game, Microseconds dt);

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
