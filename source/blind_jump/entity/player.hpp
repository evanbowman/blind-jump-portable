#pragma once

#include "blind_jump/entity/entityGroup.hpp"
#include "collision.hpp"
#include "entity.hpp"
#include "graphics/animation.hpp"
#include "number/numeric.hpp"
#include "platform/platform.hpp"
#include <optional>


class Game;

class Item;
class Twin;
class Enemy;
class Drone;
class Theif;
class OrbShot;
class SnakeHead;
class Wanderer;
class Compactor;
class WandererBigLaser;
class ConglomerateShot;

class Blaster : public Entity {
public:
    Blaster();

    void update(Platform& pfrm, Game& game, Microseconds dt, Cardinal dir);

    void shoot(Platform& pf, Game& game);

    void set_visible(bool visible);
    bool visible() const
    {
        return visible_;
    }

    // NOTE: (3,milliseconds(150)) works well
    void accelerate(u8 max_lasers, Microseconds reload_interval);
    void reset();

    void add_explosive_rounds(u8 count);

private:
    // Microseconds reload_interval_;
    Microseconds reload_ = 0;
    Cardinal dir_;
    bool visible_;
    u16 powerup_remaining_ = 0;

    // Max lasers allowed onscreen
    // u8 max_lasers_ = 2;
    // u8 explosive_rounds_ = 0;
};


class Player : public Entity {
public:
    Player(Platform& pfrm);

    void on_collision(Platform& pf, Game& game, ConglomerateShot&);
    void on_collision(Platform& pf, Game& game, WandererBigLaser&);
    void on_collision(Platform& pf, Game& game, Wanderer&);
    void on_collision(Platform& pf, Game& game, Compactor&);
    void on_collision(Platform& pf, Game& game, SnakeHead&);
    void on_collision(Platform& pf, Game& game, OrbShot&);
    void on_collision(Platform& pf, Game& game, Enemy&);
    void on_collision(Platform& pf, Game& game, Drone&);
    void on_collision(Platform& pf, Game& game, Theif&);
    void on_collision(Platform& pf, Game& game, Item&);
    void on_collision(Platform& pf, Game& game, Twin&);

    void get_score(Platform& pf, Game& game, u32 score);

    void update(Platform& pfrm, Game& game, Microseconds dt);
    void soft_update(Platform& pfrm, Game& game, Microseconds dt);
    void warp_out(Platform& pfrm, Game& game, Microseconds dt);

    inline const Sprite& get_shadow() const;
    inline const HitBox& hitbox() const;
    inline Blaster& weapon();

    inline bool is_invulnerable() const;

    void init(const Vec2<Float>& pos);

    void revive(Platform& pfrm);

    void set_visible(bool visible);

    void apply_force(const Vec2<Float>& force);

    Vec2<Float> get_speed() const;

    void heal(Platform& pfrm, Game& game, Health amount);

    Cardinal facing() const;

    int dodges() const
    {
        return dodges_;
    }

private:
    void set_sprite_texture(TextureIndex tidx);

    using ResourceLoc = TextureMap;

    Health initial_health(Platform& pfrm) const;

    template <ResourceLoc L>
    void key_response(bool k1,
                      bool k2,
                      bool k3,
                      bool k4,
                      float& speed,
                      bool collision);

    template <ResourceLoc L>
    void
    altKeyResponse(bool k1, bool k2, bool k3, float& speed, bool collision);

    template <Player::ResourceLoc S, uint8_t maxIndx>
    void on_key_released(bool k2, bool k3, bool k4, bool x);

    template <u8 StepSize>
    void update_animation(Platform& pf,
                          Microseconds dt,
                          u8 max_index,
                          Microseconds count);

    void injured(Platform& pf, Game& game, Health damage);

    void footstep(Platform& pf);

    void push_dodge_effect(Platform& pf, Game& game);

    u32 frame_;
    ResourceLoc frame_base_;
    Microseconds anim_timer_;
    Microseconds invulnerability_timer_;
    Microseconds dodge_timer_ = 0;
    Microseconds weapon_hide_timer_ = 0;
    FadeColorAnimation<Microseconds(15685)> fade_color_anim_;
    Float l_speed_;
    Float r_speed_;
    Float u_speed_;
    Float d_speed_;
    Vec2<Float> last_pos_;
    Sprite shadow_;
    HitBox hitbox_;
    std::optional<Vec2<Float>> external_force_;
    Platform::DynamicTexturePtr dynamic_texture_;

    static constexpr const int max_dodges = 1;

    u8 dodges_ = 0;

    enum class State { normal, pre_dodge, dodge } state_ = State::normal;

    Blaster blaster_;
};


const Sprite& Player::get_shadow() const
{
    return shadow_;
}


const HitBox& Player::hitbox() const
{
    return hitbox_;
}


bool Player::is_invulnerable() const
{
    return invulnerability_timer_ > 0;
}


Blaster& Player::weapon()
{
    return blaster_;
}
