#pragma once


#include "blind_jump/entity/entity.hpp"
#include "graphics/animation.hpp"


class CutsceneBird : public Entity {
public:
    CutsceneBird(const Vec2<Float>& position, int anim_start);

    void update(Platform& pfrm, Game& game, Microseconds dt);

private:
    Animation<57, 8, milliseconds(90)> animation_;

    bool was_visible_ = false;
};
