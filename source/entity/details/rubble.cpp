#include "rubble.hpp"
#include "number/random.hpp"


Rubble::Rubble(const Vec2<Float>& pos)
{
    set_position({pos.x, pos.y - 12});
    sprite_.set_position(get_position());
    sprite_.set_texture_index(TextureMap::rubble);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 0});

    if (random_choice<2>()) {
        sprite_.set_flip({true, false});
    }
}
