#include "cutsceneBird.hpp"


CutsceneBird::CutsceneBird(const Vec2<Float>& position, int anim_start)
{
    set_position(position);

    sprite_.set_position(position);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_texture_index(57 + anim_start);
}


void CutsceneBird::update(Platform& pfrm, Game& game, Microseconds dt)
{
    animation_.advance(sprite_, dt);

    if (visible()) {
        position_.y += dt * 0.000078f;
        position_.x -= dt * 0.000018f;
    }

    sprite_.set_position(position_);

    if (visible()) {
        was_visible_ = true;
    }

    if (not visible() and was_visible_) {
        this->kill();
    }
}
