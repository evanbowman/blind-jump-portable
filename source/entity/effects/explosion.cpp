#include "explosion.hpp"


Explosion::Explosion(const Vec2<Float>& position)
{
    set_position(position);

    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 8});
    sprite_.set_position(position);
}
