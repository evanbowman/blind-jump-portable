#include "orbshot.hpp"
#include "platform/platform.hpp"


OrbShot::OrbShot(const Vec2<Float>& position,
                 const Vec2<Float>& target,
                 Float speed,
                 Microseconds duration)
    : Projectile(position, target, speed),
      timer_(duration), hitbox_{&position_, {{12, 12}, {8, 8}}}
{
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 8});

    animation_.bind(sprite_);
}


AlliedOrbShot::AlliedOrbShot(const Vec2<Float>& position,
                             const Vec2<Float>& target,
                             Float speed,
                             Microseconds duration)
    : OrbShot(position, target, speed, duration)
{
    sprite_.set_mix({ColorConstant::green, 255});
}


void OrbShot::update(Platform& pf, Game& game, Microseconds dt)
{
    Projectile::update(pf, game, dt);

    timer_ -= dt;

    if (timer_ <= 0) {
        Entity::kill();
    }

    if (animation_.advance(sprite_, dt)) {
        // We don't want to prevent off-screen enemies from attacking the
        // player, but we also want to limit the number of entities that the
        // system needs to process. So if the entity's been alive for a while,
        // and is now off-screen, then it's ok to get rid of it.
        if (timer_ < milliseconds(750)) {
            if (not visible()) {
                Entity::kill();
            }
        }
    }
}


void OrbShot::on_collision(Platform& pf, Game&, Player&)
{
    Entity::kill();
    pf.sleep(5);
}
