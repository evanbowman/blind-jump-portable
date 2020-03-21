#include "laser.hpp"
#include "number/random.hpp"


static constexpr const Float move_rate = 0.0002f;


Laser::Laser(const Vec2<Float>& position, Cardinal dir)
    : dir_(dir), timer_(0), hitbox_{&position_, {16, 16}, {8, 8}}
{
    set_position(position);

    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    sprite_.set_position(position);

    sprite_.set_texture_index([this] {
        switch (dir_) {
        case Cardinal::north:
        case Cardinal::south:
            return TextureMap::v_laser;
        default:
            return TextureMap::h_laser;
        }
    }());

    if (dir_ == Cardinal::south) {
        sprite_.set_flip({false, true});
    } else if (dir_ == Cardinal::east) {
        sprite_.set_flip({true, false});
    }
}


void Laser::update(Platform& pf, Game& game, Microseconds dt)
{
    // wait till we've had one update cycle to check visibility
    if (timer_ != 0) {
        if (not this->visible()) {
            this->kill();
            return;
        }
    }

    timer_ += dt;

    if (timer_ > milliseconds(20)) {
        timer_ = 0;
        const auto flip = sprite_.get_flip();
        const auto spr = sprite_.get_texture_index();
        if (dir_ == Cardinal::north or dir_ == Cardinal::south) {
            sprite_.set_flip({static_cast<bool>(random_choice<2>()), flip.y});
        } else {
            sprite_.set_flip({flip.x, static_cast<bool>(random_choice<2>())});
        }
        if (random_choice<3>() == 0) {
            if (spr == TextureMap::v_laser) {
                sprite_.set_texture_index(TextureMap::v_laser2);
            } else if (spr == TextureMap::v_laser2) {
                sprite_.set_texture_index(TextureMap::v_laser);
            } else if (spr == TextureMap::h_laser) {
                sprite_.set_texture_index(TextureMap::h_laser2);
            } else if (spr == TextureMap::h_laser2) {
                sprite_.set_texture_index(TextureMap::h_laser);
            }
        }
    }

    switch (dir_) {
    case Cardinal::north:
        position_.y -= dt * move_rate;
        break;

    case Cardinal::south:
        position_.y += dt * move_rate;
        break;

    case Cardinal::east:
        position_.x += dt * move_rate;
        break;

    case Cardinal::west:
        position_.x -= dt * move_rate;
        break;
    }

    sprite_.set_position(position_);
}


void Laser::on_collision(Platform&, Game&, Turret&)
{
    this->kill();
}


void Laser::on_collision(Platform&, Game&, Dasher&)
{
    this->kill();
}


void Laser::on_collision(Platform&, Game&, SnakeHead&)
{
    this->kill();
}


void Laser::on_collision(Platform&, Game&, SnakeBody&)
{
    this->kill();
}


void Laser::on_collision(Platform&, Game&, Drone&)
{
    this->kill();
}
