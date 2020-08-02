#pragma once

#include "number/random.hpp"
#include "number/numeric.hpp"
#include "graphics/sprite.hpp"


namespace net_event {

struct Header {
    enum MessageType {
         player_info,
         new_level_idle,
         new_level_seed,
    } message_type_;
};


struct PlayerInfo {
    Header header_;
    Vec2<Float> position_; // TODO: Downsample to a 16 bit integer to save space
    Vec2<Float> speed_;
    u16 texture_index_;
    Sprite::Size size_;
};


struct NewLevelIdle {
    Header header_;
};


struct NewLevelSeed {
    Header header_;
    rng::Generator random_state;
};

}
