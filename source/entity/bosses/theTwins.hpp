#pragma once

#include "entity/enemies/enemy.hpp"


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
        return LocaleString::boss1_defeated;
    }

    void update(Platform& pf, Game& game, Microseconds dt);

    std::array<const Sprite*, 2> get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &head_;
        return ret;
    }

    auto get_shadow() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &shadow_;
        ret[1] = &shadow2_;
        return ret;
    }

    void on_collision(Platform&, Game&, LaserExplosion&);
    void on_collision(Platform&, Game&, AlliedOrbShot&);
    void on_collision(Platform&, Game&, Player&);
    void on_collision(Platform&, Game&, Laser&);

private:
    enum class State {
        inactive, idle, open_mouth, ranged_attack_charge, ranged_attack, ranged_attack_done
    } state_ = State::inactive;

    void injured(Platform&, Game&, Health amount);

    void update_sprite();
    void set_sprite(TextureIndex index);

    Sprite head_;
    Sprite shadow2_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Microseconds timer_ = 0;
    Microseconds alt_timer_ = 0;
};
