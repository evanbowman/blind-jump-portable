#include "orbshot.hpp"
#include "platform/platform.hpp"


OrbShot::OrbShot(const Vec2<Float>& position, const Vec2<Float>& target) :
    Projectile(position, target, 0.00015),
    timer_(0),
    hitbox_{&position_, {16, 16}, {8, 8}}
{
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 8});

    animation_.bind(sprite_);
}


void OrbShot::update(Platform& pf, Game& game, Microseconds dt)
{
    Projectile::update(pf, game, dt);

    timer_ += dt;

    if (timer_ > seconds(2)) {
        Entity::kill();
    }

    animation_.advance(sprite_, dt);
}


void OrbShot::on_collision(Platform& pf, Game&, Player&)
{
    Entity::kill();
    pf.sleep(5);
}
