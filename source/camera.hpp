#pragma once

#include "entity.hpp"
#include "numeric.hpp"

class Platform;


class Camera {
public:
    void update(Platform& pfrm, Microseconds dt, const Vec2<Float>& seek_pos);

    void set_position(Platform& pfrm, const Vec2<Float>& pos);
};
