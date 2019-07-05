#pragma once

#include "numeric.hpp"


using TextureIndex = u32;


class Sprite {
public:
    Sprite() : texture_index_(0)
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

private:
    Vec2<Float> position_;
    Vec2<bool> flip_;
    TextureIndex texture_index_;
};
