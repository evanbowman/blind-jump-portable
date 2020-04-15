#pragma once


#include "collision.hpp"
#include "entity/entity.hpp"
#include "tileMap.hpp"
#include "enemy.hpp"


class Laser;


class Scarecrow : public Enemy {
public:
    Scarecrow(const Vec2<Float>& pos);

    static constexpr bool multiface_sprite = true;

    std::array<const Sprite*, 2> get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &leg_;
        return ret;
    }

    void update(Platform&, Game&, Microseconds);

    void on_collision(Platform&, Game&, Laser&);


private:
    enum class State {
        sleep,
        idle_airborne,
        idle_wait,
        idle_crouch,
        idle_jump,
        long_crouch,
        long_wait,
        long_jump,
        long_airborne,
        landing,
    } state_ = State::idle_wait;

    Sprite leg_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Microseconds timer_;
    Microseconds bounce_timer_;
    Vec2<TIdx> anchor_;
    Vec2<Float> move_vec_;
    bool hit_;
};
