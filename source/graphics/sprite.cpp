#include "sprite.hpp"


Sprite::Sprite()
    : alpha_(Alpha::opaque), size_(Size::w32_h32), flip_x_(false),
      flip_y_(false)
{
}


void Sprite::set_scale(const Scale& scale)
{
    scale_ = scale;
}


void Sprite::set_rotation(Rotation rot)
{
    rot_ = rot;
}


void Sprite::set_position(const Vec2<Float>& position)
{
    position_ = position;
}


void Sprite::set_origin(const Vec2<s16>& origin)
{
    origin_ = origin;
}


void Sprite::set_texture_index(TextureIndex texture_index)
{
    texture_index_ = texture_index;
}


void Sprite::set_flip(const Vec2<bool>& flip)
{
    flip_x_ = flip.x;
    flip_y_ = flip.y;
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


Sprite::Rotation Sprite::get_rotation() const
{
    return rot_;
}


Sprite::Scale Sprite::get_scale() const
{
    return scale_;
}


const Vec2<Float>& Sprite::get_position() const
{
    return position_;
}


const Vec2<s16>& Sprite::get_origin() const
{
    return origin_;
}


TextureIndex Sprite::get_texture_index() const
{
    return texture_index_;
}


Vec2<bool> Sprite::get_flip() const
{
    return {flip_x_, flip_y_};
}


Sprite::Alpha Sprite::get_alpha() const
{
    return static_cast<Sprite::Alpha>(alpha_);
}


const ColorMix& Sprite::get_mix() const
{
    return mix_;
}


Sprite::Size Sprite::get_size() const
{
    return static_cast<Sprite::Size>(size_);
}
