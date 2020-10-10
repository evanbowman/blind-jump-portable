#include "particle.hpp"
#include "number/random.hpp"


Particle::Particle(const Vec2<Float>& position,
                   ColorConstant color,
                   Float drift_speed) : timer_(0)
{
    set_position(position);

    sprite_.set_texture_index(TextureMap::coin);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_mix({color, 255});
    sprite_.set_scale({-24, -24});
    sprite_.set_origin({8, 0});
    sprite_.set_position(position_);

    step_vector_ =
        direction(position_, rng::sample<64>(position_, rng::utility_state))
        * drift_speed;
}


void Particle::update(Platform& pfrm, Game& game, Microseconds dt)
{
    timer_ += dt;

    position_ = position_ + Float(dt) * step_vector_;

    static const auto duration = seconds(1);

    const s16 shrink_amount = interpolate(-450, -24, Float(timer_) / duration);

    sprite_.set_scale({shrink_amount, shrink_amount});
    sprite_.set_position(position_);

    if (timer_ > duration) {
        this->kill();
    }
}
