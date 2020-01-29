#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"
#include "number/numeric.hpp"


class Game;
class Platform;


class Critter : public Entity {
public:
    Critter(Platform& pf);

    void update(Platform&, Game&, Microseconds);
};
