#include "camera.hpp"
#include "platform/platform.hpp"


void Camera::update(Platform& pfrm,
                    Microseconds dt,
                    const Vec2<Float>& seek_pos)
{
    const auto& screen_size = pfrm.screen().size();
    auto view = pfrm.screen().get_view();
    const auto& view_center = view.get_center();

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


    static const std::array<std::array<Float, 5>,
                            static_cast<int>(ShakeMagnitude::zero)>
        shake_constants = {
            {{6.f, -10.f, 6.f, -4.f, 2.f}, {12.f, -20.f, 12.f, -8.f, 4.f}}};

    auto center = interpolate(
        target,
        view_center,
        dt * speed_ * (ballast_.divisor_ ? 0.0000016f : 0.0000071f));

    ballast_.divisor_ = 0;

    if (shake_magnitude_ == ShakeMagnitude::zero) {

        view.set_center(center);

    } else {

        shake_timer_ += dt;

        if (shake_timer_ > milliseconds(50)) {

            view.set_center({center.x,
                             center.y + shake_constants[static_cast<int>(
                                        shake_magnitude_)][shake_index_]});

            shake_timer_ = 0;
            shake_index_ += 1;

            if (shake_index_ == shake_constants[0].size()) {
                shake_index_ = 4;
                shake_magnitude_ = ShakeMagnitude::zero;
            }
        }

        // view.set_center(center);
    }

    pfrm.screen().set_view(view);

    ballast_ = {};
}


void Camera::shake(ShakeMagnitude magnitude)
{
    if (shake_magnitude_ == ShakeMagnitude::zero) {
        shake_magnitude_ = magnitude;
        shake_timer_ = 0;
        shake_index_ = 0;
    }
}


void Camera::set_position(Platform& pfrm, const Vec2<Float>& pos)
{
    auto view = pfrm.screen().get_view();

    view.set_center(pos);

    pfrm.screen().set_view(view);
}
