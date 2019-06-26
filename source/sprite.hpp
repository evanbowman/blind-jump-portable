#pragma once

#include "numeric.hpp"


class Sprite {
public:
    enum class Shape {
        square,
        wide,
        tall
    };

    enum class Size {

    };

    Sprite();

    Sprite(const Sprite&) = delete;

    ~Sprite();

    bool initialize();

    void set_position(const Vec2<float>& position);

    const Vec2<float> get_position() const;

private:
    volatile void* data_;
};
