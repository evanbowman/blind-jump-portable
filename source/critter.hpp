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
};
