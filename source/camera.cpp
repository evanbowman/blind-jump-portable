#include "camera.hpp"
#include "platform/platform.hpp"


void Camera::update(Platform& pfrm,
                    Microseconds dt,
                    const Vec2<Float>& seek_pos)
{
    const auto& screen_size = pfrm.screen().size();
    auto view = pfrm.screen().get_view();

    Vec2<Float> seek;

    if (ballast_.divisor_) {
        const auto counter_weight = ballast_.center_ / float(ballast_.divisor_);
        buffer_ = interpolate(buffer_, counter_weight, 0.000000025f * dt);
        seek = interpolate(seek_pos, buffer_, 0.5f);
    } else {
        seek = seek_pos;
    }

    Vec2<Float> target{(seek.x - screen_size.x / 2),
                       (seek.y - screen_size.y / 2)};


    // static const std::array<std::array<Float, 5>,
    //                         static_cast<int>(ShakeMagnitude::zero)>
    //     shake_constants = {
    //         {{3.f, -5.f, 3.f, -2.f, 1.f}, {6.f, -10.f, 6.f, -4.f, 2.f}}};

    center_ = interpolate(target,
                          center_,
                          dt * speed_ *
                              (ballast_.divisor_ ? 0.0000016f : 0.0000071f));

    ballast_.divisor_ = 0;

    if (shake_magnitude_ == 0) {

        view.set_center(center_);

    } else {

        shake_timer_ += dt;

        const auto shake_duration = milliseconds(250);

        if (shake_timer_ > shake_duration) {
            shake_timer_ = 0;
            shake_magnitude_ = 0;
        }

        // Exponents are too expensive, so we aren't doing true damping...
        const auto damping =
            ease_out(shake_timer_, 0, shake_magnitude_ / 2, shake_duration);

        auto offset = shake_magnitude_ * (float(cosine(shake_timer_ / 4)) /
                                          std::numeric_limits<s16>::max());

        if (offset > 0) {
            offset -= damping;
            if (offset < 0) {
                offset = 0;
            }
        } else {
            offset += damping;
            if (offset > 0) {
                offset = 0;
            }
        }

        view.set_center({center_.x, center_.y + offset});
    }

    pfrm.screen().set_view(view);

    ballast_ = {};
}


void Camera::shake(int magnitude)
{
    if (shake_magnitude_ == 0 or shake_magnitude_ < magnitude) {
        shake_magnitude_ = magnitude;
        shake_timer_ = 0;
        shake_index_ = 0;
    }
}


void Camera::set_position(Platform& pfrm, const Vec2<Float>& pos)
{
    auto view = pfrm.screen().get_view();

    view.set_center(pos);

    center_ = pos;

    pfrm.screen().set_view(view);
}
