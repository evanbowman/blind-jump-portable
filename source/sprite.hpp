#pragma once

#include "color.hpp"
#include "numeric.hpp"


using TextureIndex = u32;


class Sprite {
public:
    enum class Alpha : u8 { opaque, translucent };

    enum class Size : u8 { w32_h32, w32_h16 };

    Sprite(Size size = Size::w32_h32)
        : texture_index_(0), alpha_(Alpha::opaque), size_(size)
    {
    }

    void set_position(const Vec2<Float>& position)
    {
        position_ = position;
    }

    const Vec2<Float>& get_position() const
    {
        return position_;
    }

    void set_origin(const Vec2<s32>& origin)
    {
        origin_ = origin;
    }

    const Vec2<s32>& get_origin() const
    {
        return origin_;
    }

    void set_texture_index(TextureIndex texture_index)
    {
        texture_index_ = texture_index;
    }

    TextureIndex get_texture_index() const
    {
        return texture_index_;
    }

    void set_flip(const Vec2<bool>& flip)
    {
        flip_ = flip;
    }

    const Vec2<bool>& get_flip() const
    {
        return flip_;
    }

    Alpha get_alpha() const
    {
        return alpha_;
    }

    void set_alpha(Alpha alpha)
    {
        alpha_ = alpha;
    }

    const ColorMix& get_mix() const
    {
        return mix_;
    }

    void set_mix(const ColorMix& mix)
    {
        mix_ = mix;
    }

    Size get_size() const
    {
        return size_;
    }

    void set_size(Size size)
    {
        size_ = size;
    }

private:
    Vec2<Float> position_;
    Vec2<s32> origin_;
    Vec2<bool> flip_;
    TextureIndex texture_index_;
    Alpha alpha_;
    Size size_;
    ColorMix mix_;
};
