#pragma once

#include "entity.hpp"
#include "animation.hpp"


class Game;
class Platform;


class ItemChest : public Entity<ItemChest, 4> {
public:
    ItemChest();

    void update(Platform&, Game&, Microseconds dt);

private:
    enum class State { closed, opening, opened };

    Animation<26, 6, Microseconds(50000)> animation_;
    State state_;
};
