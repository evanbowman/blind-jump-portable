#include "cutsceneCloud.hpp"
#include "number/random.hpp"


CutsceneCloud::CutsceneCloud(const Vec2<Float>& position)
{
    set_position(position);

    const auto cloud_index = rng::choice<7>(rng::critical_state) + 2;

    sprite_.set_texture_index(cloud_index * 3 + 2);
    overflow_sprs_[0].set_texture_index(cloud_index * 3 + 1);
    overflow_sprs_[1].set_texture_index(cloud_index * 3);

    overflow_sprs_[0].set_origin({32, 0});
    overflow_sprs_[1].set_origin({64, 0});

    sprite_.set_position(position);
    overflow_sprs_[0].set_position(position);
    overflow_sprs_[1].set_position(position);


    set_speed([&] {
        switch (cloud_index) {
        case 0:
        case 1:
        case 6:
            return 0.00009f;
        case 5:
        case 2:
            return 0.00008f;
        case 4:
        case 3:
            break;
        }
        return 0.000095f;
    }());
}


void CutsceneCloud::update(Platform& pfrm, Game& game, Microseconds dt)
{
    position_.y += dt * scroll_speed_;
    // position_.x += dt * (scroll_speed_ * 0.006f);

    sprite_.set_position(position_);
    overflow_sprs_[0].set_position(position_);
    overflow_sprs_[1].set_position(position_);

    if (visible()) {
        was_visible_ = true;
    }

    if (not visible() and was_visible_) {
        this->kill();
    }
}
