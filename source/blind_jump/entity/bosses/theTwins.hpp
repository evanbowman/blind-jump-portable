#pragma once

#include "blind_jump/entity/enemies/enemy.hpp"
#include "blind_jump/localeString.hpp"
#include "blind_jump/network_event.hpp"


class LaserExplosion;
class AlliedOrbShot;
class Player;
class Laser;


class Twin : public Enemy {
public:
    Twin(const Vec2<Float>& position);

    static constexpr bool multiface_sprite = true;
    static constexpr bool multiface_shadow = true;

    constexpr bool is_allied()
    {
        return false;
    }

    inline LocaleString defeated_text() const
    {
        return LocaleString::boss2_defeated;
    }

    void update(Platform& pf, Game& game, Microseconds dt);

    Buffer<const Sprite*, 3> get_sprites() const
    {
        Buffer<const Sprite*, 3> ret;
        ret.push_back(&sprite_);
        ret.push_back(&head_);
        if (helper_.sprite_.get_texture_index() == 65) {
            ret.push_back(&helper_.sprite_);
        }
        return ret;
    }

    auto get_shadow() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &shadow_;
        ret[1] = &shadow2_;
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
    void on_collision(Platform&, Game&, Player&);
    void on_collision(Platform&, Game&, Laser&);

    void on_death(Platform&, Game&);

private:
    enum class State {
        inactive,
        long_idle,
        idle,
        open_mouth,
        ranged_attack_charge,
        ranged_attack,
        ranged_attack_done,
        prep_leap,
        crouch,
        leaping,
        landing,
        prep_mutation,
        mutate,
        mutate_done,
        mode2_long_idle,
        mode2_idle,
        mode2_prep_move,
        mode2_start_move,
        mode2_moving,
        mode2_stop_move,
        mode2_stop_friction,
        mode2_open_mouth,
        mode2_ranged_attack_charge,
        mode2_ranged_attack,
        mode2_ranged_attack_done,
    } state_ = State::inactive;

    struct Helper {
        enum class State {
            recharge,
            shoot1,
            shoot2,
            shoot3
        } state_ = State::recharge;
        Sprite sprite_;
        Microseconds timer_ = 0;
        Vec2<Float> target_;

        void update(Platform& pfrm,
                    Game& game,
                    Microseconds dt,
                    const Entity& parent,
                    const Entity& target);
    };

    Twin* sibling(Game& game);

    void injured(Platform&, Game&, Health amount);

    void update_sprite();
    void set_sprite(TextureIndex index);

    Helper helper_;
    Sprite head_;
    Sprite shadow2_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Microseconds timer_ = 0;
    Microseconds alt_timer_ = 0;
    Vec2<Float> speed_;
    Vec2<Float> target_;
    s8 leaps_ = 0;
    s8 shadow_offset_ = 2;
};
