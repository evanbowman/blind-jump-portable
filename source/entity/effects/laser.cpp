#include "laser.hpp"


static constexpr const Float move_rate = 0.0002f;


Laser::Laser(const Vec2<Float>& position, Direction dir)
    : dir_(dir), timer_(0), hitbox_{&position_, {16, 16}, {8, 8}}
{
    set_position(position);

    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 8});
    sprite_.set_position(position);

    sprite_.set_texture_index([this] {
        switch (dir_) {
        case Direction::up:
        case Direction::down:
            return TextureMap::v_laser;
        default:
            return TextureMap::h_laser;
        }
    }());
}


void Laser::update(Platform& pf, Game& game, Microseconds dt)
{
    // wait till we've had one update cycle to check visibility
    if (timer_ == 0) {
        timer_ += dt;
    } else {
        if (not this->visible()) {
            this->kill();
            return;
        }
    }

    switch (dir_) {
    case Direction::up:
        position_.y -= dt * move_rate;
        break;

    case Direction::down:
        position_.y += dt * move_rate;
        break;

    case Direction::left:
        position_.x -= dt * move_rate;
        break;

    case Direction::right:
        position_.x += dt * move_rate;
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


void Laser::on_collision(Platform&, Game&, Laser&)
{
    this->kill();
}
