#include "probe.hpp"


Probe::Probe(const Vec2<Float>& position)
    : hitbox_{&position_, {{16, 16}, {8, 8}}}
{
    position_ = position;
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 8});

    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, 8});
    shadow_.set_alpha(Sprite::Alpha::translucent);
}


void Probe::update(Platform&, Game&, Microseconds dt)
{
}
