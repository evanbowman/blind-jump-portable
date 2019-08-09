#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"
#include "numeric.hpp"


class Game;
class Platform;


class Critter : public Entity<Critter, 20> {
public:
    Critter();

    void update(Platform&, Game&, Microseconds);

    // void on_collision(Platform& pf, Game& game,Critter&) override;
};
