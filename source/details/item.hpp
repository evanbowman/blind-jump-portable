#pragma once

#include "collision.hpp"
#include "entity.hpp"


class Player;


class Item : public Entity<Item, 12> {
public:
    enum class Type { heart, coin };

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