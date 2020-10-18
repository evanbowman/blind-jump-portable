#include "particle.hpp"
#include "number/random.hpp"


Particle::Particle(const Vec2<Float>& position,
                   ColorConstant color,
                   Float drift_speed,
                   Microseconds duration,
                   Microseconds offset,
                   std::optional<Float> angle,
                   const Vec2<Float>& reference_speed)
    : timer_(offset), duration_(duration)
{
    set_position(position);

    sprite_.set_texture_index(TextureMap::coin);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_mix({color, 255});
    sprite_.set_scale({-24, -24});
    sprite_.set_origin({8, 0});
    sprite_.set_position(position_);

    if (angle) {
        step_vector_ = cartesian_angle(*angle) * drift_speed;
    } else {
        step_vector_ =
            direction(position_,
                      rng::sample<64>(position_, rng::utility_state)) *
            drift_speed;
    }

    step_vector_ = step_vector_ + reference_speed;
}


void Particle::update(Platform& pfrm, Game& game, Microseconds dt)
{
    timer_ += dt;

    position_ = position_ + Float(dt) * step_vector_;

    const s16 shrink_amount = interpolate(-450, -24, Float(timer_) / duration_);

    sprite_.set_scale({shrink_amount, shrink_amount});
    sprite_.set_position(position_);

    if (timer_ > duration_) {
        this->kill();
    }
}
