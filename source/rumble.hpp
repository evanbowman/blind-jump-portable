#pragma once

#include "number/numeric.hpp"
#include "platform/platform.hpp"


class Rumble {
public:
    void update(Platform& pfrm, Microseconds dt)
    {
        if (duration_ > 0) {
            duration_ -= dt;
            if (duration_ <= 0) {
                duration_ = 0;
                enabled_ = false;
                pfrm.keyboard().rumble(false);
            }
        }
    }

    void activate(Platform& pfrm, Microseconds duration)
    {
        if (not enabled_) {
            pfrm.keyboard().rumble(true);
        }
        duration_ = std::max(duration, duration_);
        enabled_ = true;
    }

private:
    bool enabled_ = false;
    Microseconds duration_ = 0;
};
