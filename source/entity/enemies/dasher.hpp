#pragma once

#include "collision.hpp"
#include "entity/entity.hpp"
#include "graphics/sprite.hpp"
#include "enemy.hpp"


class Player;
class Laser;


class Dasher : public Enemy {
public:
    Dasher(const Vec2<Float>& position);

    void update(Platform&, Game&, Microseconds);

    static constexpr bool multiface_sprite = true;

    std::array<const Sprite*, 2> get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &head_;
        return ret;
    }

    void on_collision(Platform&, Game&, Laser&);
    void on_collision(Platform&, Game&, Player&) {}

private:
    enum class State {
        sleep,
        inactive,
        idle,
        shot1,
        shot2,
        shot3,
        dash_begin,
        dashing,
        dash_end,
        shoot_begin,
        pause
    };

    Sprite head_;
    Microseconds timer_;
    State state_;
    Vec2<Float> speed_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
