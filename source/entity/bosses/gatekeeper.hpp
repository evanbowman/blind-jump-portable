#pragma once

#include "entity/enemies/enemy.hpp"


class LaserExplosion;
class Player;
class Laser;


class Gatekeeper : public Enemy {
public:
    Gatekeeper(const Vec2<Float>& position, Game& game);

    void update(Platform&, Game&, Microseconds);

    static constexpr bool multiface_sprite = true;
    static constexpr bool has_shadow = true;

    auto get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &head_;
        return ret;
    }

    void on_collision(Platform&, Game&, LaserExplosion&);
    void on_collision(Platform&, Game&, Laser&);
    void on_collision(Platform&, Game&, Player&)
    {
    }

    void on_death(Platform&, Game&);

    int shield_radius() const
    {
        return shield_radius_;
    }

    int shield_radius2() const
    {
        return shield_radius2_;
    }

    bool second_form() const
    {
        return not third_form() and get_health() < 80;
    }

    bool third_form() const
    {
        return get_health() < 45;
    }

private:
    void injured(Platform&, Game&, Health amount);

    enum class State {
        sleep,
        idle,
        jump,
        airborne,
        landing,
        prep_shield_sweep,
        shield_sweep_out,
        shield_sweep_in1,
        shield_sweep_in2,

        second_form_enter,
        encircle_receede,

        third_form_enter,
        third_form_sweep_in,
    };

    Sprite head_;
    State state_;
    Microseconds timer_;
    Microseconds charge_timer_;
    Vec2<Float> move_vec_;
    int shield_radius_;
    int shield_radius2_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};


class GatekeeperShield : public Enemy {
public:
    enum class State { orbit, encircle, detached };

    GatekeeperShield(const Vec2<Float>& position,
                     int offset,
                     State initial_state = State::orbit);

    void update(Platform&, Game&, Microseconds);

    void on_collision(Platform&, Game&, Laser&);
    void on_collision(Platform&, Game&, Player&)
    {
    }
    void on_collision(Platform&, Game&, LaserExplosion&)
    {
    }

    void on_death(Platform&, Game&);

    void detach(Microseconds keepalive);

private:
    Microseconds timer_;
    Microseconds reload_;
    int shot_count_;
    State state_;
    int offset_;
    Vec2<Float> target_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
