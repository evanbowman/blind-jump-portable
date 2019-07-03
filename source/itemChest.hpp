#pragma once

#include "entity.hpp"
#include "animation.hpp"


class Game;
class Platform;


class ItemChest : public Entity<ItemChest, 4> {
public:
    ItemChest()
    {
        sprite_.set_position({50.f, 50.f});
    }
    
    void update(Platform&, Game&, Microseconds dt)
    {
        (void)dt;
        animation_.advance(sprite_, 0);
    }

    const Sprite& get_sprite() const
    {
        return sprite_;
    }
    
private:
    Sprite sprite_;
    Animation<26, 6, Microseconds(50000)> animation_;
};
