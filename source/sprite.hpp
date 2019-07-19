#pragma once

#include "color.hpp"
#include "numeric.hpp"


using TextureIndex = u32;


class Sprite {
public:
    enum class Alpha : u8 { opaque, translucent };

    enum class Size : u8 { w32_h32, w16_h32 };

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


// NOTE: If you see two texture indices with the same value, don't
// panic! The program supports both 32px and 16px wide sprites, so two
// of the same indices might refer to different sprites due to
// different sprite sizes.
enum TextureMap : TextureIndex {
    player_still_down = 0,
    player_walk_down = 1,
    player_walk_up = 6,
    player_still_up = 11,
    player_walk_left = 12,
    player_still_left = 18,
    player_walk_right = 19,
    player_still_right = 25,
    item_chest = 12,
    turret = 18,
    turret_shadow = 23,
};
