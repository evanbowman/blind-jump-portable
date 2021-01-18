#pragma once

#include "blind_jump/entity/entity.hpp"
#include "item.hpp"
#include "memory/buffer.hpp"


class Scavenger : public Entity {
public:
    Scavenger(const Vec2<Float>& pos);

    static constexpr bool has_shadow = true;

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

    Buffer<Item::Type, 16> inventory_;

private:
    Sprite shadow_;
};
