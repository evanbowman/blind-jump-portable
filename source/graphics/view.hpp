#pragma once

#include "number/numeric.hpp"


class View {
public:
    void set_center(const Vec2<Float>& center);

    void set_size(const Vec2<Float>& size);

    const Vec2<Float>& get_center() const;

    const Vec2<Float>& get_size() const;

private:
    Vec2<Float> center_;
    Vec2<Float> size_;
};
