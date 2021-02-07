#pragma once

#include "blind_jump/entity/entity.hpp"
#include "blind_jump/network_event.hpp"
#include "collision.hpp"
#include "enemy.hpp"
#include "graphics/sprite.hpp"


class LaserExplosion;
class AlliedOrbShot;
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

    using Enemy::on_collision;

    void on_collision(Platform&, Game&, Laser&);
    void on_collision(Platform&, Game&, LaserExplosion&);
    void on_collision(Platform&, Game&, AlliedOrbShot&);

    void on_death(Platform&, Game&);

    void sync(const net_event::EnemyStateSync&, Game&);

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

    void injured(Platform&, Game&, Health amount);

    Sprite head_;
    Microseconds timer_;
    State state_;
    Vec2<Float> speed_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
