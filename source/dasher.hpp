#pragma once

#include "sprite.hpp"
#include "entity.hpp"


class Game;
class Platform;


class Dasher : public Entity<Dasher, 20> {
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
