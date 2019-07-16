#pragma once

#include "entity.hpp"
#include "numeric.hpp"
#include "sprite.hpp"
#include "collision.hpp"


class Game;
class Platform;


class Critter : public Entity<Critter, 20>,
                public CollidableTemplate<Critter> {
public:
    Critter()
    {
        sprite_.set_texture_index(0); // FIXME
    }

    void update(Platform&, Game&, Microseconds)
    {
    }
};
