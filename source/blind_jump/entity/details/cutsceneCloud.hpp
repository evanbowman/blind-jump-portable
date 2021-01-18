#pragma once

#include "blind_jump/entity/entity.hpp"


class CutsceneCloud : public Entity {
public:
    CutsceneCloud(const Vec2<Float>& position);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    static constexpr bool multiface_sprite = true;

    auto get_sprites() const
    {
        std::array<const Sprite*, 3> ret;
        ret[0] = &sprite_;
        ret[1] = &overflow_sprs_[0];
        ret[2] = &overflow_sprs_[1];

        return ret;
    }

    void set_speed(Float speed)
    {
        scroll_speed_ = speed;
    }

private:
    Sprite overflow_sprs_[2];
    Float scroll_speed_ = 0.f;
    bool was_visible_ = false;
};
