#pragma once

#include "platform.hpp"
#include "entity.hpp"


class Game;


class Dasher : public Entity<Dasher, 20> {
public:

    inline void update(Platform&, Game&, Microseconds)
    {

    }
};
