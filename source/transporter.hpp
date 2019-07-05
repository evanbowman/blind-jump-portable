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
        sprite_.set_position({140, 80});
    }

    void update(Platform&, Game&, Microseconds)
    {
        // ...
    }
};
