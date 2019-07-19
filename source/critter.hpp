#pragma once

#include "collision.hpp"
#include "entity.hpp"
#include "numeric.hpp"
#include "sprite.hpp"


class Game;
class Platform;


class Critter : public Entity<Critter, 20> {
public:
    Critter();

    void update(Platform&, Game&, Microseconds);

    // void receive_collision(Critter&) override;
};
