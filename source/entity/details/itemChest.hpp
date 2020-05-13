#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"
#include "graphics/animation.hpp"
#include "item.hpp"


class ItemChest : public Entity {
public:
    ItemChest(const Vec2<Float>& pos, Item::Type item);

    void update(Platform&, Game&, Microseconds dt);

    static constexpr bool has_shadow = true;

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

    enum class State { closed, opening, settle, opened };

    State state() const
    {
        return state_;
    }

private:
    Animation<TextureMap::item_chest, 6, Microseconds(50000)> animation_;
    Sprite shadow_;
    State state_;
    Item::Type item_;
};
