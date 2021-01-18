#pragma once


#include "blind_jump/entity/entity.hpp"


class Reticule : public Entity {
public:
    Reticule(const Vec2<Float>& position)
    {
        set_position(position);

        sprite_.set_size(Sprite::Size::w32_h32);
        sprite_.set_texture_index(62);
        sprite_.set_origin({16, 16});
        sprite_.set_position(position);
    }

    void update(Platform&, Game&, Microseconds)
    {
    }

    void move(const Vec2<Float>& position)
    {
        sprite_.set_position(position);
        set_position(position);
    }

    void destroy()
    {
        kill();
    }
};
