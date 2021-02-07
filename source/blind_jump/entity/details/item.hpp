#pragma once

#include "blind_jump/entity/entity.hpp"
#include "collision.hpp"
#include "localization.hpp"


class Player;


class Item : public Entity {
public:
    enum class Type : u8 {
        null = 0,
        heart = 1,
        coin = 2,
        inventory_item_start = 3,
        worker_notebook_1 = inventory_item_start,
        blaster = 4,
        accelerator = 5,
        lethargy = 6,
        old_poster_1 = 7,
        map_system = 8,
        explosive_rounds_2 = 9,
        seed_packet = 10,
        engineer_notebook_2 = 11,
        signal_jammer = 12,
        navigation_pamphlet = 13,
        orange = 14,
        orange_seeds = 15,
        long_jump_z2 = 16,

        // Items which repurpose existing item icons should be placed below this
        // line. Other items, with original icons, should be placed above this
        // line.
        long_jump_z3 = 17,
        long_jump_z4 = 18,
        postal_advert = 19,
        engineer_notebook_1 = 20,
        worker_notebook_2 = 21,
        long_jump_home = 22,
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

    void set_type(Type type);

    void update(Platform&, Game&, Microseconds dt);

    void scatter();

    bool ready() const;

private:
    enum class State { idle, scatter } state_;

    Microseconds timer_;
    Type type_;
    HitBox hitbox_;
    Vec2<Float> step_;
};


// Some items, mainly story-related, persist across game sessions. Persistent
// items should also be unique.
inline bool item_is_persistent(Item::Type type)
{
    return type == Item::Type::blaster or
           type == Item::Type::worker_notebook_1 or
           type == Item::Type::worker_notebook_2 or
           type == Item::Type::old_poster_1 or type == Item::Type::map_system or
           type == Item::Type::seed_packet or
           type == Item::Type::engineer_notebook_1 or
           type == Item::Type::engineer_notebook_2 or
           type == Item::Type::navigation_pamphlet or
           type == Item::Type::orange_seeds or
           type == Item::Type::postal_advert;
}


// Note: For memory efficiency, this function happens to be implemented in
// state.cpp, so that the implementation can pull the data from an existing
// readonly table containing a variety of Item metadata used by the inventory
// state.
LocalizedText item_description(Platform&, Item::Type type);
