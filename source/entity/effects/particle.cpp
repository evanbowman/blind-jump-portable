#include "particle.hpp"
#include "number/random.hpp"


Particle::Particle(const Vec2<Float>& position,
                   std::optional<ColorConstant> color,
                   Float drift_speed,
                   bool x_wave_effect,
                   Microseconds duration,
                   Microseconds offset,
                   std::optional<Float> angle,
                   const Vec2<Float>& reference_speed)
    : timer_(offset), duration_(duration), x_wave_effect_(x_wave_effect)
{
    set_position(position);

    sprite_.set_texture_index(TextureMap::coin);
    sprite_.set_size(Sprite::Size::w16_h32);
    if (color) {
        sprite_.set_mix({*color, 255});
    }
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

    const Float offset = [&] {
        if (x_wave_effect_) {
            return 3 * float(sine(4 * 3.14f * 0.005f * timer_ + 180)) /
                   std::numeric_limits<s16>::max();
        } else {
            return 0.f;
        }
    }();

    sprite_.set_position({position_.x + offset, position_.y});

    if (timer_ > duration_) {
        this->kill();
    }
}
