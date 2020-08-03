#include "rubble.hpp"
#include "number/random.hpp"


Rubble::Rubble(const Vec2<Float>& pos)
{
    const auto temp1 = pos.cast<s32>();
    const auto temp2 = temp1.cast<Float>();
    set_position({temp2.x, temp2.y - 12});
    sprite_.set_position(get_position());
    sprite_.set_texture_index(TextureMap::rubble);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 0});

    if (rng::choice<2>(rng::utility_state)) {
        sprite_.set_flip({true, false});
    }
}
