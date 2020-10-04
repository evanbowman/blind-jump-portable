#include "terminal.hpp"


Terminal::Terminal(const Vec2<Float>& position)
{
    position_ = position;

    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_position(position);
    sprite_.set_origin({8, 16});

    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_position(position);
    shadow_.set_origin({8, 16});

    shadow_.set_alpha(Sprite::Alpha::translucent);


    sprite_.set_texture_index(111);
    shadow_.set_texture_index(109);
}


void Terminal::update(Platform&, Game&, Microseconds dt)
{
    // TODO: slow this flicker down, for screens with refresh rates faster than
    // 60hz.
    if (sprite_.get_texture_index() == 111) {
        sprite_.set_texture_index(110);
    } else {
        sprite_.set_texture_index(111);
    }
}
