#pragma once

#include "animation.hpp"
#include "collision.hpp"
#include "entity.hpp"


class ItemChest : public Entity<ItemChest, 4> {
public:
    ItemChest(const Vec2<Float>& pos);

    void update(Platform&, Game&, Microseconds dt);

private:
    enum class State { closed, opening, settle, opened };

    Animation<TextureMap::item_chest, 6, Microseconds(50000)> animation_;
    State state_;
};
