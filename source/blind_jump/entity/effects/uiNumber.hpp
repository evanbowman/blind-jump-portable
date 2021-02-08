#pragma once

#include "blind_jump/entity/entity.hpp"


class UINumber : public Entity {
public:
    UINumber(const Vec2<Float>& position, s8 val, Id parent);

    static constexpr bool multiface_sprite = true;

    std::array<const Sprite*, 3> get_sprites() const
    {
        std::array<const Sprite*, 3> ret;
        ret[0] = &sprite_;
        ret[1] = &extra_;
        ret[2] = &sign_;
        return ret;
    }


    void update(Platform& pf, Game& game, Microseconds dt);

    void set_val(s8 val);

    s8 get_val() const;

    void hold(Microseconds duration)
    {
        hold_ = duration;
    }

private:
    Sprite sign_;
    Sprite extra_;
    Microseconds timer_;
    Microseconds hold_;
    Id parent_;
    Float offset_ = 0.f;
};
