#pragma once

#include "graphics/animation.hpp"
#include "collision.hpp"
#include "entity.hpp"
#include "number/numeric.hpp"


class Game;
class Platform;
class Critter;
class Dasher;
class Turret;
class Probe;
class Item;
class OrbShot;


class Player : public Entity {
public:
    Player();

    void on_collision(Platform& pf, Game& game, OrbShot&);
    void on_collision(Platform& pf, Game& game, Critter&);
    void on_collision(Platform& pf, Game& game, Dasher&);
    void on_collision(Platform& pf, Game& game, Turret&);
    void on_collision(Platform& pf, Game& game, Probe&);
    void on_collision(Platform& pf, Game& game, Item&);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void soft_update(Platform& pfrm, Game& game, Microseconds dt);

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

    bool is_invulnerable() const
    {
        return invulnerability_timer_ > 0;
    }

    void move(const Vec2<Float>& pos);

    void revive();

private:
    using ResourceLoc = TextureMap;

    template <ResourceLoc L>
    void key_response(bool k1,
                      bool k2,
                      bool k3,
                      bool k4,
                      float& speed,
                      bool collision);

    template <Player::ResourceLoc S, uint8_t maxIndx>
    void on_key_released(bool k2, bool k3, bool k4, bool x);

    template <u8 StepSize>
    void update_animation(Microseconds dt, u8 max_index, Microseconds count);

    void injured(Platform& pf, Health damage);

    u32 frame_;
    ResourceLoc frame_base_;
    Microseconds anim_timer_;
    Microseconds color_timer_;
    Microseconds invulnerability_timer_;
    Float l_speed_;
    Float r_speed_;
    Float u_speed_;
    Float d_speed_;
    Sprite shadow_;
    HitBox hitbox_;
};
