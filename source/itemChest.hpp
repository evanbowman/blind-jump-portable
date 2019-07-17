#pragma once

#include "animation.hpp"
#include "collision.hpp"
#include "entity.hpp"


class Game;
class Platform;


class ItemChest : public Entity<ItemChest, 4> {
public:
    ItemChest(const Vec2<Float>& pos);

    void update(Platform&, Game&, Microseconds dt);

private:
    enum class State { closed, opening, settle, opened };

    Animation<26, 6, Microseconds(50000)> animation_;
    State state_;
};
