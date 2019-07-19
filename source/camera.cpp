#include "camera.hpp"
#include "platform.hpp"


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
        seek = interpolate(seek_pos, buffer_, 0.78f);
    } else {
        seek = seek_pos;
    }

    Vec2<Float> target{(seek.x - screen_size.x / 2),
                       (seek.y - screen_size.y / 2)};

    view.set_center(interpolate(target, view_center, dt * 0.00000125f));

    pfrm.screen().set_view(view);

    ballast_ = {};
}


void Camera::set_position(Platform& pfrm, const Vec2<Float>& pos)
{
    auto view = pfrm.screen().get_view();

    view.set_center(pos);

    pfrm.screen().set_view(view);
}
