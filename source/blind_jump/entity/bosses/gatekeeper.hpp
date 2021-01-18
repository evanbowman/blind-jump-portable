#pragma once

#include "blind_jump/entity/enemies/enemy.hpp"
#include "blind_jump/localeString.hpp"
#include "blind_jump/network_event.hpp"


class LaserExplosion;
class AlliedOrbShot;
class Player;
class Laser;


class Gatekeeper : public Enemy {
public:
    Gatekeeper(const Vec2<Float>& position, Game& game);

    void update(Platform&, Game&, Microseconds);

    static constexpr bool multiface_sprite = true;
    static constexpr bool has_shadow = true;

    inline LocaleString defeated_text() const
    {
        return LocaleString::boss1_defeated;
    }

    constexpr bool is_allied()
    {
        return false;
    }

    auto get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &head_;
        return ret;
    }

    void sync(const net_event::EnemyStateSync&, Game&)
    {
        // TODO: Bosses unsupported in multiplayer games.
        while (true)
            ;
    }

    void on_collision(Platform&, Game&, LaserExplosion&);
    void on_collision(Platform&, Game&, AlliedOrbShot&);
    void on_collision(Platform&, Game&, Laser&);

    using Enemy::on_collision;

    void on_death(Platform&, Game&);

    int shield_radius() const
    {
        return shield_radius_;
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
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};


class GatekeeperShield : public Enemy {
public:
    // enum class State { orbit, encircle, detached };

    class AttackPattern {
    public:
        virtual ~AttackPattern()
        {
        }

        virtual void update(GatekeeperShield& shield,
                            Platform& pfrm,
                            Game& game,
                            Microseconds dt) = 0;
    };

    GatekeeperShield(const Vec2<Float>& position,
                     int offset // ,
                                // State initial_state = State::orbit
    );

    ~GatekeeperShield();


    void sync(const net_event::EnemyStateSync&, Game&)
    {
        // TODO: Bosses unsupported in multiplayer games.
        while (true)
            ;
    }


    void update(Platform&, Game&, Microseconds);

    void on_collision(Platform&, Game&, Laser&);

    using Enemy::on_collision;

    void on_collision(Platform&, Game&, LaserExplosion&)
    {
    }

    void on_collision(Platform&, Game&, AlliedOrbShot&);

    void set_offset(int offset)
    {
        offset_ = offset;
    }

    void on_death(Platform&, Game&);

    void detach(Microseconds keepalive);

    void set_speed(Float speed)
    {
        speed_ = speed;
    }

    void open();
    void close();

    static constexpr const int AttackPatternBufAlign = 8;

    template <typename Pattern, typename... Args>
    void set_attack_pattern(Args&&... args)
    {
        static_assert(sizeof(Pattern) <= sizeof(attack_pattern_));
        static_assert(alignof(Pattern) <= AttackPatternBufAlign);

        if (attack_pattern_set_) {
            ((AttackPattern*)attack_pattern_)->~AttackPattern();
        }

        attack_pattern_set_ = true;
        new ((Pattern*)attack_pattern_) Pattern(std::forward<Args>(args)...);
    }


    void attack(Platform& pfrm, Game& game, Microseconds dt);

private:
    Microseconds timer_;
    Microseconds shadow_update_timer_;
    bool detached_;
    bool opened_;
    int offset_;
    Float speed_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    alignas(AttackPatternBufAlign) u8 attack_pattern_[32];
    bool attack_pattern_set_ = false;
};
