#pragma once

#include "numeric.hpp"
#include "sprite.hpp"


template <TextureIndex InitialTexture, u32 Length, Microseconds Interval>
class Animation {
public:

    Animation() :
        timer_(0),
        texture_index_(InitialTexture)
    {

    }

    bool done() const
    {
        return texture_index_ == (InitialTexture + (Length - 1));
    }

    void advance(Sprite& sprite, Microseconds dt)
    {
        timer_ += dt;
        if (timer_ > Interval) {
            timer_ -= Interval;
            if (not Animation::done()) {
                texture_index_ += 1;
            } else {
                // Note: all animations wrap.
                texture_index_ = InitialTexture;
            }
        }
        sprite.set_texture_index(texture_index_);
    }

private:
    Microseconds timer_;
    TextureIndex texture_index_;
};
