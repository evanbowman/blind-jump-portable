#pragma once



#include "entity/entity.hpp"
#include "tileMap.hpp"



class Snake : public Entity {
public:
    Snake(const Vec2<Float>& pos, Game& game);

    Snake(const Vec2<Float>& pos, Snake* parent, Game&, u8 remaining);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

private:

    void init(const Vec2<Float>& pos);

    Sprite shadow_;
    Snake* parent_;

    Vec2<TIdx> tile_coord_;
};
