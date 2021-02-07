#pragma once

#include "blind_jump/entity/effects/dialogBubble.hpp"
#include "blind_jump/entity/entity.hpp"
#include "blind_jump/network_event.hpp"
#include "collision.hpp"
#include "graphics/animation.hpp"
#include "item.hpp"


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
