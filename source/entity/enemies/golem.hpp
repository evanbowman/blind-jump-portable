#pragma once


#include "collision.hpp"
#include "enemy.hpp"


class LaserExplosion;
class AlliedOrbShot;
class Player;
class Laser;


class Golem : public Enemy {
public:
    Golem(const Vec2<Float>& pos);

    using Enemy::on_collision;

    void on_collision(Platform&, Game&, AlliedOrbShot&);
    void on_collision(Platform&, Game&, LaserExplosion&);
    void on_collision(Platform&, Game&, Laser&);

    void update(Platform&, Game&, Microseconds dt);

    void on_death(Platform&, Game&);

    static constexpr bool multiface_sprite = true;

    std::array<const Sprite*, 2> get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &head_;
        return ret;
    }

    enum class State {
        sleep,
        inactive,
        follow,
        charge,
        shooting,
        pause,
    };

    void sync(const net_event::EnemyStateSync& state, Game&);

private:
    void injured(Platform&, Game&, Health amount);

    void follow();

    Sprite head_;
    State state_;
    u8 count_;
    Vec2<Float> speed_;
    Vec2<Float> target_;
    Microseconds timer_;
    Microseconds anim_timer_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
