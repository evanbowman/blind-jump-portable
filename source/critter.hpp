#pragma once

#include "platform.hpp"
#include "entity.hpp"


class Game;


class Critter : public Entity<Critter, 20> {
public:
    
    void update(const Platform&, Game&, Microseconds)
    {
        
    }
};
