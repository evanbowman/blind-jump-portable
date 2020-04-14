#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"
#include "graphics/animation.hpp"
#include "item.hpp"


class ItemChest : public Entity {
public:
    ItemChest(const Vec2<Float>& pos, Item::Type item);

    void update(Platform&, Game&, Microseconds dt);

private:
    enum class State { closed, opening, settle, opened };

    Animation<TextureMap::item_chest, 6, Microseconds(50000)> animation_;
    // FadeColorAnimation<milliseconds(20)> fade_color_anim_;
    State state_;
    Item::Type item_;
};
