#pragma once

#include "numeric.hpp"
#include "entity.hpp"
#include "animation.hpp"


class Game;
class Platform;


class Player : public Entity<Player, 1> {
public:
    Player();

    void update(Platform& pfrm, Game& game, Microseconds dt);

    const Sprite& get_sprite() const;

    const Vec2<Float>& get_position() const
    {
        return sprite_.get_position();
    }

private:
    enum class State {
         inactive,
         normal
    };

    enum TextureIndex {
         still_up    = 11,
         still_down  = 0,
         still_left  = 18,
         still_right = 25,
         walk_up     = 6,
         walk_down   = 1,
         walk_left   = 26,
         walk_right  = 19,
    };

    template <TextureIndex T>
    void key_response(bool k1,
                      bool k2,
                      bool k3,
                      bool k4,
                      bool collision);

    Sprite sprite_;
    u32 frame_;
    State state_;
    TextureIndex texture_index_;
    Microseconds anim_timer_;
};
