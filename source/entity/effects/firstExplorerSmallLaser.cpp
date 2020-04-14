#include "firstExplorerSmallLaser.hpp"
#include "number/random.hpp"


FirstExplorerSmallLaser::FirstExplorerSmallLaser(const Vec2<Float>& position,
                                                 const Vec2<Float>& target,
                                                 Float speed)
    : OrbShot(position, target, speed)
{
}


void FirstExplorerSmallLaser::update(Platform& pf, Game& game, Microseconds dt)
{
    OrbShot::update(pf, game, dt);

    flicker_timer_ += dt;
    if (flicker_timer_ > milliseconds(40)) {
        switch (random_choice<2>()) {
        case 0:
            sprite_.set_mix({ColorConstant::electric_blue, 255});
            break;

        case 1:
            sprite_.set_mix({ColorConstant::picton_blue, 255});
            break;
        }
    }
}
