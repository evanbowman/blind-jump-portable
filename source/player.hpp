#pragma once

#include "animation.hpp"
#include "collision.hpp"
#include "entity.hpp"
#include "numeric.hpp"


class Game;
class Platform;


class Player : public Entity<Player, 0>, public CollidableTemplate<Player> {
public:
    Player();

    void receive_collision(Critter&) override;
    void receive_collision(Dasher&) override;
    void receive_collision(Probe&) override;

    void update(Platform& pfrm, Game& game, Microseconds dt);

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

private:
    enum ResourceLoc {
        still_up = 11,
        still_down = 0,
        still_left = 15,
        still_right = 22,
        walk_up = 6,
        walk_down = 1,
        walk_left = 9,
        walk_right = 16,
    };

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

    u32 frame_;
    ResourceLoc frame_base_;
    Microseconds anim_timer_;
    float l_speed_;
    float r_speed_;
    float u_speed_;
    float d_speed_;
    Sprite shadow_;
    u16 health_;
};
