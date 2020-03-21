#include "item.hpp"
#include "number/random.hpp"
#include "platform/platform.hpp"


Item::Item(const Vec2<Float>& pos, Platform&, Type type)
    : timer_(random_value()), type_(type), hitbox_{&position_, {16, 16}, {8, 8}}
{
    position_ = pos - Vec2<Float>{8, 0};
    sprite_.set_position(position_);
    switch (type) {
    case Type::heart:
        sprite_.set_texture_index(TextureMap::heart);
        break;

    case Type::null:
    case Type::coin:
        sprite_.set_texture_index(TextureMap::coin);
        break;
    }
    sprite_.set_size(Sprite::Size::w16_h32);
}


void Item::on_collision(Platform& pf, Game&, Player&)
{
    Entity::kill();
    pf.sleep(5);
}


void Item::update(Platform&, Game&, Microseconds dt)
{
    timer_ += dt;

    if (visible()) {
        // Yikes! This is probably expensive...
        const Float offset = 3 * float(sine(4 * 3.14 * 0.001f * timer_ + 180)) /
                             std::numeric_limits<s16>::max();
        sprite_.set_position({position_.x, position_.y + offset});
    }
}
