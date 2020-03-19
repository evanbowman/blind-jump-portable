#include "explosion.hpp"


Explosion::Explosion(const Vec2<Float>& position)
{
    set_position(position);

    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 8});
    sprite_.set_position(position);

    animation_.bind(sprite_);
}


void Explosion::update(Platform& pfrm, Game& game, Microseconds dt)
{
    if (animation_.advance(sprite_, dt) and animation_.done(sprite_)) {
        this->kill();
    }
}
