#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"
#include "sprite.hpp"


class Player;


class Dasher : public Entity<Dasher, 20> {
public:
    Dasher(const Vec2<Float>& position);

    void update(Platform&, Game&, Microseconds);

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

    static constexpr bool multiface_sprite = true;

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

private:
    enum class State {
        inactive,
        idle,
        shooting,
        dash_begin,
        dashing,
        dash_end,
        shoot_begin,
        pause
    };

    Sprite shadow_;
    Sprite head_;
    HitBox hitbox_;
    Microseconds timer_;
    State state_;
};
