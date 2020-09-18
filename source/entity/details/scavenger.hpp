#pragma once

#include "entity/entity.hpp"
#include "item.hpp"
#include "memory/buffer.hpp"


class Scavenger : public Entity {
public:
    Scavenger(const Vec2<Float>& pos);

    Buffer<Item::Type, 16> inventory_;
};
