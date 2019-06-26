#include "animation.hpp"


Animation::Animation(Microseconds interval, u32 length) :
    timer_(0),
    interval_(interval),
    length_(length),
    keyframe_(0)
{
    
}


bool Animation::advance(Microseconds dt) 
{
    timer_ += dt;
    if (timer_ > interval_) {
        timer_ -= interval_;
        keyframe_ += 1;
        return true;
    }
    return false;
}


bool Animation::done() const
{
    return keyframe_ == length_ - 1;
}
