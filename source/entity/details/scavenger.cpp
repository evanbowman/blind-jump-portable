#include "scavenger.hpp"


Scavenger::Scavenger(const Vec2<Float>& pos)
{
    position_ = pos;

    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_position(pos);
    sprite_.set_origin({16, 16});
    sprite_.set_texture_index(62);
}
