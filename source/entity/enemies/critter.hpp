#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"
#include "number/numeric.hpp"


class Game;
class Platform;


class Critter : public Entity<Critter, 20> {
public:
    Critter(Platform& pf);

    void update(Platform&, Game&, Microseconds);
};
