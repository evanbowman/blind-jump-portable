#include "scavenger.hpp"
#include "number/random.hpp"


Scavenger::Scavenger(const Vec2<Float>& pos)
{
    position_ = pos;

    position_.y -= 4;

    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_position(position_);
    sprite_.set_origin({16, 16});
    sprite_.set_texture_index(57);

    shadow_.set_size(Sprite::Size::w32_h32);
    shadow_.set_position(position_);
    shadow_.set_origin({16, 14});
    shadow_.set_texture_index(56);
    shadow_.set_alpha(Sprite::Alpha::translucent);

    if (rng::choice<2>(rng::utility_state)) {
        sprite_.set_flip({true, false});
        shadow_.set_flip({true, false});
    }
}
