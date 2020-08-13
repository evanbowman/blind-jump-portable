#pragma once

#include "entity/entity.hpp"

class Lander : public Entity {
public:
    Lander();

    static constexpr bool multiface_sprite = true;

    void update(Platform& pfrm, Game& game, Microseconds dt);

    std::array<const Sprite*, 2> get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &spr2_;
        return ret;
    }

    bool done() const;

private:
    Microseconds timer_;
    int anim_index_;
    Sprite spr2_;
};
