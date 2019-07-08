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

    const Vec2<Float>& get_position() const
    {
        return sprite_.get_position();
    }

    void update(Platform&, Game&, Microseconds)
    {
        // ...
    }
};
