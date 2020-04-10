#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"


class Player;


class Item : public Entity {
public:
    enum class Type : u8 {
        null,
        heart,
        coin,
        surveyor_logbook,
        blaster,
        accelerator,
        lethargy,
        count
    };

    Item(const Vec2<Float>& pos, Platform&, Type type);

    void on_collision(Platform& pf, Game& game, Player&);

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

    Type get_type() const
    {
        return type_;
    }

    void update(Platform&, Game&, Microseconds dt);

private:
    Microseconds timer_;
    Type type_;
    HitBox hitbox_;
};


// Some items, mainly story-related, persist across game sessions. Persistent
// items should also be unique.
inline bool item_is_persistent(Item::Type type)
{
    return type == Item::Type::blaster or type == Item::Type::surveyor_logbook;
}


// Note: For memory efficiency, this function happens to be implemented in
// state.cpp, so that the implementation can pull the data from an existing
// readonly table containing a variety of Item metadata used by the inventory
// state.
const char* item_description(Item::Type type);
