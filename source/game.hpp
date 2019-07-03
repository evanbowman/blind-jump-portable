#pragma once

#include "platform.hpp"
#include "transformGroup.hpp"
#include "critter.hpp"
#include "dasher.hpp"
#include "player.hpp"


template <typename Arg>
using EntityBuffer = Buffer<Arg*, Arg::spawn_limit()>;


template <typename ...Args>
using EntityGroup = TransformGroup<EntityBuffer<Args>...>;


class Game {
public:
    Game();

    void update(Platform& platform, Microseconds delta);

private:
    Player* player_;
    EntityGroup<Critter, Dasher> enemies_;
};
