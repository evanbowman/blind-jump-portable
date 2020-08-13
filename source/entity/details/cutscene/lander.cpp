#include "lander.hpp"


static const Vec2<Float> initial_position{77, -12};
static const Vec2<Float> target_position{77, 86};


Lander::Lander() : timer_(0), anim_index_(0)
{
    set_position(initial_position);

    sprite_.set_position(position_);
    spr2_.set_position({position_.x, position_.y + 32});

    spr2_.set_texture_index(2);
}


void Lander::update(Platform& pfrm, Game& game, Microseconds dt)
{
    timer_ += dt;

    if (timer_ > milliseconds(50)) {
        timer_ = 0;

        switch (anim_index_) {
        case 0:
            sprite_.set_texture_index(1);
            spr2_.set_texture_index(3);
            anim_index_ = 1;
            break;

        case 1:
            sprite_.set_texture_index(0);
            spr2_.set_texture_index(2);
            anim_index_ = 0;
            break;
        }
    }

    position_.y = interpolate(target_position.y, position_.y, dt * 0.000001f);


    sprite_.set_position(position_);
    spr2_.set_position({position_.x, position_.y + 32});
}


bool Lander::done() const
{
    return abs(position_.y - target_position.y) < 1;
}
