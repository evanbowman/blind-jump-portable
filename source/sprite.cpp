#include "sprite.hpp"


Sprite::Sprite(Size size)
    : texture_index_(0), alpha_(Alpha::opaque), size_(size)
{
}


void Sprite::set_position(const Vec2<Float>& position)
{
    position_ = position;
}


void Sprite::set_origin(const Vec2<s32>& origin)
{
    origin_ = origin;
}


void Sprite::set_texture_index(TextureIndex texture_index)
{
    texture_index_ = texture_index;
}


void Sprite::set_flip(const Vec2<bool>& flip)
{
    flip_ = flip;
}


void Sprite::set_alpha(Alpha alpha)
{
    alpha_ = alpha;
}


void Sprite::set_mix(const ColorMix& mix)
{
    mix_ = mix;
}


void Sprite::set_size(Size size)
{
    size_ = size;
}


const Vec2<Float>& Sprite::get_position() const
{
    return position_;
}


const Vec2<s32>& Sprite::get_origin() const
{
    return origin_;
}


TextureIndex Sprite::get_texture_index() const
{
    return texture_index_;
}


const Vec2<bool>& Sprite::get_flip() const
{
    return flip_;
}


Sprite::Alpha Sprite::get_alpha() const
{
    return alpha_;
}


const ColorMix& Sprite::get_mix() const
{
    return mix_;
}


Sprite::Size Sprite::get_size() const
{
    return size_;
}
