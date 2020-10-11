#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"
#include "graphics/animation.hpp"
#include "item.hpp"
#include "network_event.hpp"
#include "entity/effects/dialogBubble.hpp"


class ItemChest : public Entity {
public:
    ItemChest(const Vec2<Float>& pos, Item::Type item, bool locked = true);

    void update(Platform&, Game&, Microseconds dt);

    static constexpr bool has_shadow = true;

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

    enum class State {
        closed_locked,
        closed_unlocked,
        opening,
        settle,
        opened,
        sync_opening,
        sync_settle,
    };

    State state() const
    {
        return state_;
    }

    void sync(Platform& pfrm, const net_event::ItemChestOpened&);

private:
    Animation<TextureMap::item_chest, 6, Microseconds(50000)> animation_;
    Sprite shadow_;
    State state_;
    Item::Type item_;
};
