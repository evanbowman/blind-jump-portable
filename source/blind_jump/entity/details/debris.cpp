#include "debris.hpp"
#include "number/random.hpp"


Debris::Debris(const Vec2<Float>& position) : timer_(0)
{
    set_position(rng::sample<6>(position, rng::utility_state));

    sprite_.set_position(position_);
    sprite_.set_size(Sprite::Size::w16_h32);

    if (rng::choice<2>(rng::utility_state)) {
        sprite_.set_texture_index(69);
    } else {
        sprite_.set_texture_index(70);
    }

    rotation_dir_ = rng::choice<2>(rng::utility_state);

    if (rotation_dir_) {
        sprite_.set_rotation(std::numeric_limits<s16>::max());
    }

    sprite_.set_origin({8, 16});

    static const auto drift_speed = 0.0000138f;

    step_vector_ =
        direction(position_, rng::sample<64>(position_, rng::utility_state)) *
        drift_speed;
}


void Debris::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (not visible() and timer_ > 0) {
        kill();
    }


    timer_ += delta;

    if (rotation_dir_) {
        sprite_.set_rotation(sprite_.get_rotation() - delta * 0.005);
    } else {
        sprite_.set_rotation(sprite_.get_rotation() + delta * 0.005);
    }

    position_ = position_ + Float(delta) * step_vector_;

    sprite_.set_position(position_);

    if (timer_ > seconds(3)) {
        sprite_.set_alpha(Sprite::Alpha::translucent);
    }
    if (timer_ > seconds(5)) {
        this->kill();
    }
}
