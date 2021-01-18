#include "wandererBigLaser.hpp"
#include "number/random.hpp"
#include "platform/platform.hpp"


WandererBigLaser::WandererBigLaser(const Vec2<Float>& position,
                                   const Vec2<Float>& target,
                                   Float speed)
    : Projectile(position, target, speed), timer_(0),
      flicker_timer_(0), hitbox_{&position_, {{18, 18}, {16, 16}}}
{
    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_origin({16, 16});

    animation_.bind(sprite_);
}


void WandererBigLaser::update(Platform& pf, Game& game, Microseconds dt)
{
    Projectile::update(pf, game, dt);

    timer_ += dt;
    flicker_timer_ += dt;

    if (flicker_timer_ > milliseconds(20)) {
        flicker_timer_ = 0;

        switch (rng::choice<3>(rng::utility_state)) {
        case 0:
            sprite_.set_mix({});
            break;

        case 1:
            sprite_.set_mix({ColorConstant::picton_blue, 128});
            break;

        case 2:
            sprite_.set_mix({ColorConstant::picton_blue, 255});
            break;
        }
    }

    if (timer_ > seconds(2)) {
        Entity::kill();
    }

    animation_.advance(sprite_, dt);
}


void WandererBigLaser::on_collision(Platform& pf, Game&, Player&)
{
    Entity::kill();
    pf.sleep(5);
}
