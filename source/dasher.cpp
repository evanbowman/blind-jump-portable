#include "dasher.hpp"
#include "game.hpp"


Dasher::Dasher(const Vec2<Float>& position)
    : CollidableTemplate(HitBox{&position_, {16, 32}, {8, 16}})
{
    position_ = position;

    sprite_.set_texture_index(TextureMap::dasher_idle);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});

    head_.set_texture_index(TextureMap::dasher_head);
    head_.set_size(Sprite::Size::w16_h32);
    head_.set_origin({8, 32});

    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -9});
    shadow_.set_alpha(Sprite::Alpha::translucent);
}


void Dasher::update(Platform&, Game& game, Microseconds dt)
{
    sprite_.set_position(position_);
    shadow_.set_position(position_);
    head_.set_position({position_.x, position_.y - 9});

    if (game.get_player().get_position().x > position_.x) {
        sprite_.set_flip({1, 0});
        head_.set_flip({1, 0});
    } else {
        sprite_.set_flip({0, 0});
        head_.set_flip({0, 0});
    }
}
