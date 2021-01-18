#include "dialogBubble.hpp"


DialogBubble::DialogBubble(const Vec2<Float>& position, Entity& owner)
    : state_(State::animate_in), timer_(0), owner_(&owner)
{
    set_position(position);

    sprite_.set_position(position);
    sprite_.set_texture_index(118);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    sprite_.set_scale({-450, -450});
}


void DialogBubble::update(Platform& pfrm, Game& game, Microseconds dt)
{
    switch (state_) {
    case State::animate_in: {
        timer_ += dt;
        static const auto duration = milliseconds(200);

        const s16 scale = interpolate(0, -450, Float(timer_) / duration);
        sprite_.set_scale({scale, scale});

        const auto y_offset = interpolate(6, 0, Float(timer_) / duration);
        sprite_.set_position({position_.x, position_.y - y_offset});

        if (timer_ > duration) {
            timer_ = 0;
            state_ = State::wait;
            sprite_.set_scale({0, 0});
        }
        break;
    }

    case State::wait:
        break;

    case State::animate_out: {
        timer_ += dt;
        static const auto duration = milliseconds(120);

        const s16 scale = interpolate(-450, 0, Float(timer_) / duration);
        sprite_.set_scale({scale, scale});

        const auto y_offset = interpolate(0, 6, Float(timer_) / duration);
        sprite_.set_position({position_.x, position_.y - y_offset});

        if (timer_ > duration) {
            this->kill();
        }
        break;
    }
    }
}


void DialogBubble::try_destroy(Entity& entity)
{
    if (&entity == owner_ and state_ == State::wait) {
        state_ = State::animate_out;
    }
}
