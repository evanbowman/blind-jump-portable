#pragma once

#include "numeric.hpp"
#include "sprite.hpp"


template <TextureIndex InitialTexture, u32 Length, Microseconds Interval>
class Animation {
public:
    Animation() : timer_(0), texture_index_(InitialTexture)
    {
    }

    bool done() const
    {
        return texture_index_ == (InitialTexture + (Length - 1));
    }

    constexpr TextureIndex initial_texture()
    {
        return InitialTexture;
    }

    bool advance(Sprite& sprite, Microseconds dt)
    {
        timer_ += dt;
        const bool ret = [&] {
            if (timer_ > Interval) {
                timer_ -= Interval;
                if (not Animation::done()) {
                    texture_index_ += 1;
                } else {
                    // Note: all animations wrap.
                    texture_index_ = InitialTexture;
                }
                return true;
            }
            return false;
        }();
        sprite.set_texture_index(texture_index_);
        return ret;
    }

    bool reverse(Sprite& sprite, Microseconds dt)
    {
        timer_ += dt;
        const bool ret = [&] {
            if (timer_ > Interval) {
                timer_ -= Interval;
                if (texture_index_ >= InitialTexture) {
                    texture_index_ -= 1;
                } else {
                    // Note: all animations wrap.
                    texture_index_ = InitialTexture + Length - 1;
                }
                return true;
            }
            return false;
        }();
        sprite.set_texture_index(texture_index_);
        return ret;
    }

private:
    Microseconds timer_;
    TextureIndex texture_index_;
};
