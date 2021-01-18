#include "wandererSmallLaser.hpp"
#include "number/random.hpp"


WandererSmallLaser::WandererSmallLaser(const Vec2<Float>& position,
                                       const Vec2<Float>& target,
                                       Float speed)
    : OrbShot(position, target, speed),
      flicker_timer_(rng::choice<milliseconds(40)>(rng::utility_state))
{
}


void WandererSmallLaser::update(Platform& pf, Game& game, Microseconds dt)
{
    OrbShot::update(pf, game, dt);

    flicker_timer_ += dt;
    if (flicker_timer_ > milliseconds(40)) {
        switch (rng::choice<2>(rng::utility_state)) {
        case 0:
            sprite_.set_mix({ColorConstant::electric_blue, 255});
            break;

        case 1:
            sprite_.set_mix({ColorConstant::picton_blue, 255});
            break;
        }
    }
}
