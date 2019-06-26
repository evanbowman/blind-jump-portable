#pragma once

#include "platform.hpp"


class Animation {
public:
    Animation(Microseconds interval, u32 length);

    bool advance(Microseconds dt);
    
    bool done() const;
    
private:
    Microseconds timer_;
    const Microseconds interval_;
    const u32 length_;
    Sprite spr_;
    u32 keyframe_;
};
