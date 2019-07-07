#pragma once

#include "entity.hpp"
#include "sprite.hpp"


class Game;
class Platform;


class Dasher : public Entity<Dasher, 20> {
public:
    void update(Platform&, Game&, Microseconds)
    {
    }
};
