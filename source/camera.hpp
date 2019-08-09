#pragma once

#include "entity/entity.hpp"
#include "number/numeric.hpp"

class Platform;


class Camera {
public:
    void update(Platform& pfrm, Microseconds dt, const Vec2<Float>& seek_pos);

    void set_position(Platform& pfrm, const Vec2<Float>& pos);

    // The camera supports a counter-weight to the seek position.
    void push_ballast(const Vec2<Float>& pos)
    {
        ballast_.divisor_ += 1;
        ballast_.center_ = ballast_.center_ + pos;
    }

private:
    struct Ballast {
        u32 divisor_ = 0;
        Vec2<Float> center_;
    } ballast_;

    Vec2<Float> buffer_;
};
