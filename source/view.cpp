#include "view.hpp"


void View::set_center(const Vec2<Float>& center)
{
    center_ = center;
}


void View::set_size(const Vec2<Float>& size)
{
    size_ = size;
}


const Vec2<Float>& View::get_center() const
{
    return center_;
}


const Vec2<Float>& View::get_size() const
{
    return size_;
}
