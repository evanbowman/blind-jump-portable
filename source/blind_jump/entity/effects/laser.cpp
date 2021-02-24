#include "laser.hpp"
#include "blind_jump/game.hpp"
#include "number/random.hpp"


static constexpr const Float move_rate = 0.0002f;


Laser::Laser(const Vec2<Float>& position, Cardinal dir, Mode mode)
    : dir_(dir), timer_(0), hitbox_{&position_, {{16, 16}, {8, 8}}}, mode_(mode)
{
    set_position(position);

    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    sprite_.set_position(position);

    if (mode == Mode::explosive) {
        sprite_.set_mix({ColorConstant::rich_black, 200});
    }

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

    mark_visible(true); // NOTE: This is just a hack to fix obscure bugs...
}


void Laser::update(Platform& pf, Game& game, Microseconds dt)
{
    // wait till we've had one update cycle to check visibility
    timer_ += dt;

    if (timer_ > milliseconds(20)) {
        if (not this->visible()) {
            this->kill();
            return;
        }
    }

    if (timer_ > milliseconds(20)) {
        timer_ = 0;
        const auto flip = sprite_.get_flip();
        // const auto spr = sprite_.get_texture_index();
        if (dir_ == Cardinal::north or dir_ == Cardinal::south) {
            sprite_.set_flip(
                {static_cast<bool>(rng::choice<2>(rng::utility_state)),
                 flip.y});
        } else {
            sprite_.set_flip(
                {flip.x,
                 static_cast<bool>(rng::choice<2>(rng::utility_state))});
        }
        if (mode_ not_eq Mode::explosive) {
            if (rng::choice<3>(rng::utility_state) == 0) {
                auto& info = zone_info(game.level());
                if (sprite_.get_mix().color_ == info.energy_glow_color_2_) {
                    sprite_.set_mix({});
                } else {
                    sprite_.set_mix({info.energy_glow_color_2_, 255});
                }
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


class LaserExplosion {
};


void Laser::handle_collision(Platform& pfrm, Game& game)
{
    switch (mode_) {
    case Mode::normal:
        break;

    case Mode::explosive:
        LaserExplosion exp;

        big_explosion(pfrm, game, position_);
        game.enemies().transform([&, this](auto& buf) {
            for (auto& entity : buf) {
                if (distance(position_, entity->get_position()) < 70) {
                    entity->on_collision(pfrm, game, exp);
                }
            }
        });
        break;
    }
}
