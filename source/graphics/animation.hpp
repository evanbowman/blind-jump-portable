#pragma once

#include "number/numeric.hpp"
#include "sprite.hpp"


template <TextureIndex InitialTexture, u32 Length, Microseconds Interval>
class Animation {
public:
    Animation() : timer_(0)
    {
    }

    Animation(Microseconds timer) : timer_(timer)
    {
    }

    bool done(Sprite& sprite) const
    {
        return sprite.get_texture_index() == (InitialTexture + (Length - 1));
    }

    bool at_beginning(Sprite& sprite) const
    {
        return sprite.get_texture_index() == InitialTexture;
    }

    constexpr TextureIndex initial_texture()
    {
        return InitialTexture;
    }

    void bind(Sprite& sprite)
    {
        sprite.set_texture_index(InitialTexture);
        timer_ = 0;
    }

    bool advance(Sprite& sprite, Microseconds dt)
    {
        timer_ += dt;
        const bool ret = [&] {
            if (timer_ > Interval) {
                timer_ -= Interval;
                if (not Animation::done(sprite)) {
                    sprite.set_texture_index(sprite.get_texture_index() + 1);
                } else {
                    // Note: all animations wrap.
                    sprite.set_texture_index(InitialTexture);
                }
                return true;
            }
            return false;
        }();
        return ret;
    }

    bool reverse(Sprite& sprite, Microseconds dt)
    {
        timer_ += dt;
        const bool ret = [&] {
            if (timer_ > Interval) {
                timer_ -= Interval;
                const auto current = sprite.get_texture_index();
                if (current >= InitialTexture) {
                    sprite.set_texture_index(current - 1);
                } else {
                    // Note: all animations wrap.
                    sprite.set_texture_index(InitialTexture + Length - 1);
                }
                return true;
            }
            return false;
        }();
        return ret;
    }

private:
    Microseconds timer_;
};
