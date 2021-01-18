#include "dynamicEffect.hpp"


DynamicEffect::DynamicEffect(const Vec2<Float>& position,
                             Platform::DynamicTexturePtr texture,
                             Microseconds interval,
                             u16 texture_start,
                             u8 frame_count,
                             UpdateCallback update_callback)
    : texture_(texture), update_callback_(update_callback), timer_(0),
      interval_(interval), texture_start_(texture_start),
      frame_count_(frame_count)
{
    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_position(position);
    texture_->remap(texture_start * 2);
    sprite_.set_texture_index(texture_start);
    sprite_.set_origin({16, 16});
}


void DynamicEffect::update(Platform& pfrm, Game& game, Microseconds dt)
{
    timer_ += dt;
    if (timer_ > interval_) {
        timer_ = 0;
        auto tid = sprite_.get_texture_index();
        if (tid == texture_start_ + frame_count_) {
            this->kill();
            return;
        }

        tid += 1;
        texture_->remap(tid * 2);
        sprite_.set_texture_index(tid);
    }

    update_callback_(pfrm, game, dt, *this);
}
