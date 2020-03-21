#pragma once

#include "collision.hpp"
#include "entity.hpp"
#include "entity/entityGroup.hpp"
#include "graphics/animation.hpp"
#include "number/numeric.hpp"


class Game;
class Platform;
class Critter;
class Dasher;
class Turret;
class Drone;
class Item;
class OrbShot;
class SnakeHead;
class SnakeBody;


class Blaster : public Entity {
public:
    Blaster();

    void update(Platform& pfrm, Game& game, Microseconds dt, Cardinal dir);

    void shoot(Platform& pf, Game& game);

    void set_visible(bool visible);

private:
    Microseconds reload_ = 0;
    Cardinal dir_;
    bool visible_;
};


class Player : public Entity {
public:
    Player();

    void on_collision(Platform& pf, Game& game, SnakeHead&);
    void on_collision(Platform& pf, Game& game, SnakeBody&);
    void on_collision(Platform& pf, Game& game, OrbShot&);
    void on_collision(Platform& pf, Game& game, Critter&);
    void on_collision(Platform& pf, Game& game, Dasher&);
    void on_collision(Platform& pf, Game& game, Turret&);
    void on_collision(Platform& pf, Game& game, Drone&);
    void on_collision(Platform& pf, Game& game, Item&);

    void update(Platform& pfrm, Game& game, Microseconds dt);
    void soft_update(Platform& pfrm, Game& game, Microseconds dt);

    inline const Sprite& get_shadow() const;
    inline const HitBox& hitbox() const;
    inline Blaster& weapon();

    inline bool is_invulnerable() const;

    void move(const Vec2<Float>& pos);

    void revive();

    void set_visible(bool visible);

private:
    using ResourceLoc = TextureMap;

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
    void update_animation(Microseconds dt, u8 max_index, Microseconds count);

    void injured(Platform& pf, Health damage);

    u32 frame_;
    ResourceLoc frame_base_;
    Microseconds anim_timer_;
    Microseconds invulnerability_timer_;
    FadeColorAnimation<Microseconds(15685)> fade_color_anim_;
    Float l_speed_;
    Float r_speed_;
    Float u_speed_;
    Float d_speed_;
    Sprite shadow_;
    HitBox hitbox_;

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
