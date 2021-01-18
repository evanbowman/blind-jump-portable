#pragma once

#include "blind_jump/entity/enemies/enemy.hpp"
#include "blind_jump/localeString.hpp"
#include "blind_jump/network_event.hpp"


class LaserExplosion;
class AlliedOrbShot;
class Player;
class Laser;


class InfestedCore : public Enemy {
public:
    InfestedCore(const Vec2<Float>& position);

    static constexpr bool multiface_sprite = true;
    static constexpr bool has_shadow = true;

    Buffer<const Sprite*, 2> get_sprites() const
    {
        Buffer<const Sprite*, 2> ret;
        ret.push_back(&sprite_);
        ret.push_back(&top_);
        return ret;
    }


    inline LocaleString defeated_text() const
    {
        return LocaleString::boss3_cores_defeated;
    }

    bool is_allied() const
    {
        return false;
    }

    void sync(const net_event::EnemyStateSync&, Game&)
    {
        // TODO: Bosses unsupported in multiplayer games.
        while (true)
            ;
    }

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void on_collision(Platform&, Game&, LaserExplosion&)
    {
    }
    void on_collision(Platform&, Game&, AlliedOrbShot&)
    {
    }
    void on_collision(Platform&, Game&, Player&)
    {
    }
    void on_collision(Platform&, Game&, Laser&);

private:
    void injured(Platform&, Game&, Health amount);

    enum class State {
        sleep,
        active,
        spawn_1,
        spawn_2,
        spawn_3,
        spawn_4,
        await,
        vulnerable,
    } state_;

    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Microseconds anim_timer_;
    Microseconds spawn_timer_;
    Sprite top_;
};
