#pragma once

#include "blind_jump/entity/entity.hpp"


class Lander : public Entity {
public:
    Lander(const Vec2<Float>& position);

    static constexpr bool multiface_sprite = true;
    static constexpr bool multiface_shadow = true;
    static constexpr bool has_shadow = true;

    auto get_sprites() const
    {
        std::array<const Sprite*, 4> ret;
        ret[0] = &sprite_;
        ret[1] = &overflow_sprs_[0];
        ret[2] = &overflow_sprs_[1];
        ret[3] = &overflow_sprs_[2];
        return ret;
    }

    auto get_shadow() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &shadow_[0];
        ret[1] = &shadow_[1];
        return ret;
    }

private:
    Sprite overflow_sprs_[3];
    Sprite shadow_[2];
};
