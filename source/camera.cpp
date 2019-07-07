#include "camera.hpp"
#include "platform.hpp"


void Camera::update(Platform& pfrm,
                    Microseconds dt,
                    const Vec2<Float>& seek_pos)
{
    const auto& screen_size = pfrm.screen().size();
    auto view = pfrm.screen().get_view();
    const auto& view_center = view.get_center();

    Vec2<Float> target{(seek_pos.x - screen_size.x / 2) + 16,
                       (seek_pos.y - screen_size.y / 2) + 16};

    view.set_center(interpolate(target, view_center, dt * 0.0000025f));

    pfrm.screen().set_view(view);
}
