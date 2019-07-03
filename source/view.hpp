#pragma once

#include "numeric.hpp"


class View {
public:
    void set_center(const Vec2<Float>& center)
    {
        center_ = center;
    }

    const Vec2<Float>& get_center() const
    {
        return center_;
    }

    void set_size(const Vec2<Float>& size)
    {
        size_ = size;
    }

    const Vec2<Float>& get_size() const
    {
        return size_;
    }

private:
    Vec2<Float> center_;
    Vec2<Float> size_;
};
