#pragma once

#include "entity.hpp"
#include "sprite.hpp"


class Game;
class Platform;


class Transporter : public Entity<Transporter, 0> {
public:
    Transporter()
    {
        sprite_.set_texture_index(32);
    }

    void set_position(const Vec2<Float>& pos)
    {
        sprite_.set_position(pos);
    }

    void update(Platform&, Game&, Microseconds)
    {
        // ...
    }
};
