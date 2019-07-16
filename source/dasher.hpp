#pragma once

#include "entity.hpp"
#include "sprite.hpp"
#include "collision.hpp"


class Game;
class Platform;


class Dasher : public Entity<Dasher, 20>,
               public CollidableTemplate<Dasher> {
public:
    Dasher()
    {
        sprite_.set_texture_index(0); // FIXME
    }

    void update(Platform&, Game&, Microseconds)
    {
    }
};
