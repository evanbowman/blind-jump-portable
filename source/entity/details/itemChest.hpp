#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"
#include "graphics/animation.hpp"


class ItemChest : public Entity {
public:
    ItemChest(const Vec2<Float>& pos);

    void update(Platform&, Game&, Microseconds dt);

    inline void unlock()
    {
        state_ = State::closed;
    }

private:
    enum class State { locked, closed, opening, settle, opened };

    Animation<TextureMap::item_chest, 6, Microseconds(50000)> animation_;
    FadeColorAnimation<milliseconds(20)> fade_color_anim_;
    State state_;
};
