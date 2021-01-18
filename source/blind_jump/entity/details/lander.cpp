#include "lander.hpp"


Lander::Lander(const Vec2<Float>& position)
{
    set_position(position);

    sprite_.set_texture_index(42);
    sprite_.set_origin({32, 32});
    sprite_.set_position(position);

    overflow_sprs_[0].set_origin({0, 32});
    overflow_sprs_[1].set_origin({32, 0});

    for (int i = 0; i < 3; ++i) {
        overflow_sprs_[i].set_texture_index(43 + i);
        overflow_sprs_[i].set_position(position);
    }

    shadow_[0].set_texture_index(46);
    shadow_[0].set_origin({32, 0});
    shadow_[0].set_position(position);
    shadow_[0].set_alpha(Sprite::Alpha::translucent);

    shadow_[1].set_texture_index(47);
    shadow_[1].set_position(position);
    shadow_[1].set_alpha(Sprite::Alpha::translucent);
}
