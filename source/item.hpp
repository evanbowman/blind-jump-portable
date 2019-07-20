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

    void update(Platform&, Game&, Microseconds dt)
    {
        timer_ += dt;
        // Yikes! This is probably expensive...
        const Float offset = 3 * float(sine(4 * 3.14 * 0.001f * timer_ + 180)) /
                             std::numeric_limits<s16>::max();
        sprite_.set_position({position_.x, position_.y + offset});
    }

private:
    Microseconds timer_;
    Type type_;
    HitBox hitbox_;
};
