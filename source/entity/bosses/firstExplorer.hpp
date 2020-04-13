#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"


class Player;
class Laser;


class FirstExplorer : public Entity {
public:
    FirstExplorer(const Vec2<Float>& position);

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

    static constexpr bool multiface_sprite = true;

    void update(Platform& pf, Game& game, Microseconds dt);

    std::array<const Sprite*, 2> get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &head_;
        return ret;
    }

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

    void on_collision(Platform&, Game&, Player&)
    {
    }

    void on_collision(Platform&, Game&, Laser&);

private:
    enum class State {
        sleep,
        still,
        draw_weapon,
        shooting,
        prep_dash,
        dash,
    } state_ = State::sleep;

    Sprite head_;
    Sprite shadow_;
    HitBox hitbox_;
    Microseconds timer_;
    Microseconds timer2_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
