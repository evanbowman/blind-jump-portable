#include "item.hpp"
#include "platform.hpp"


Item::Item(const Vec2<Float>& pos, Platform& pf, Type type)
    : timer_(pf.random()), type_(type),
      hitbox_{&position_, {16, 16}, {8, 8}}
{
    position_ = pos;
    sprite_.set_position(pos);
    switch (type) {
    case Type::heart:
        sprite_.set_texture_index(TextureMap::heart);
        break;

    case Type::coin:
        sprite_.set_texture_index(TextureMap::coin);
        break;
    }
    sprite_.set_size(Sprite::Size::w16_h32);
}


void Item::on_collision(Platform& pf, Game& game, Player&)
{
    Entity::kill();
    pf.sleep(5);
}
