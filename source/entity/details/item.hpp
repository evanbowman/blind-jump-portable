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
        old_poster_1,
        map_system,
        explosive_rounds_2,
        seed_packet,
        engineer_notebook,
        signal_jammer,
        navigation_pamphlet,
        orange,
        orange_seeds,
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
           type == Item::Type::surveyor_logbook or
           type == Item::Type::old_poster_1 or type == Item::Type::map_system or
           type == Item::Type::seed_packet or
           type == Item::Type::engineer_notebook or
           type == Item::Type::navigation_pamphlet or
           type == Item::Type::orange_seeds;
}


// Note: For memory efficiency, this function happens to be implemented in
// state.cpp, so that the implementation can pull the data from an existing
// readonly table containing a variety of Item metadata used by the inventory
// state.
const char* item_description(Item::Type type);
