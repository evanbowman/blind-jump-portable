#pragma once

#include "numeric.hpp"
#include "entity.hpp"
#include "animation.hpp"


class Game;
class Platform;


class Player : public Entity<Player, 0> {
public:
    Player();

    void update(Platform& pfrm, Game& game, Microseconds dt);

    const Vec2<Float>& get_position() const
    {
        return sprite_.get_position();
    }

private:

    enum ResourceLoc {
         still_up    = 11,
         still_down  = 0,
         still_left  = 18,
         still_right = 25,
         walk_up     = 6,
         walk_down   = 1,
         walk_left   = 12,
         walk_right  = 19,
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

    void update_animation(Microseconds dt, u8 max_index, Microseconds count);

    u32 frame_;
    ResourceLoc frame_base_;
    Microseconds anim_timer_;
    float l_speed;
    float r_speed;
    float u_speed;
    float d_speed;
};
