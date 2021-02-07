#pragma once

#include "blind_jump/settings.hpp"
#include "number/numeric.hpp"

class Platform;


class Camera {
public:
    void update(Platform& pfrm,
                Settings::CameraMode mode,
                Microseconds dt,
                const Vec2<Float>& seek_pos);

    void set_position(Platform& pfrm, const Vec2<Float>& pos);

    void shake(int magnitude = 12);

    // The camera supports a counter-weight to the seek position.
    void push_ballast(const Vec2<Float>& pos)
    {
        ballast_.divisor_ += 1;
        ballast_.center_ = ballast_.center_ + pos;
    }

    void set_speed(Float speed)
    {
        speed_ = speed;
    }

    Vec2<Float> center() const
    {
        return center_;
    }

private:
    struct Ballast {
        u32 divisor_ = 0;
        Vec2<Float> center_;
    } ballast_;

    Vec2<Float> center_;

    Vec2<Float> buffer_;
    Float speed_ = 1.f;

    int shake_magnitude_ = 0;

    Microseconds shake_timer_ = 0;
};
