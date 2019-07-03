#pragma once

#include "numeric.hpp"
#include "entity.hpp"
#include "sprite.hpp"


class Game;
class Platform;


class Critter : public Entity<Critter, 20> {
public:

    void update(Platform&, Game&, Microseconds)
    {

    }

    const Sprite& get_sprite() const
    {
        return sprite_;
    }

private:
    Sprite sprite_;
};
