#include "dasher.hpp"
#include "../game.hpp"
#include "../random.hpp"


Dasher::Dasher(const Vec2<Float>& position)
    : hitbox_{&position_, {16, 32}, {8, 16}}, timer_(0), state_(State::idle)
{
    position_ = position;

    sprite_.set_texture_index(TextureMap::dasher_idle);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});

    head_.set_texture_index(TextureMap::dasher_head);
    head_.set_size(Sprite::Size::w16_h32);
    head_.set_origin({8, 32});

    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -9});
    shadow_.set_alpha(Sprite::Alpha::translucent);
}


void Dasher::update(Platform& pf, Game& game, Microseconds dt)
{
    sprite_.set_position(position_);
    shadow_.set_position(position_);
    head_.set_position({position_.x, position_.y - 9});

    auto face_player = [this, &player = game.get_player()] {
        if (player.get_position().x > position_.x) {
            sprite_.set_flip({1, 0});
            head_.set_flip({1, 0});
        } else {
            sprite_.set_flip({0, 0});
            head_.set_flip({0, 0});
        }
    };

    timer_ += dt;

    const auto& screen_size = pf.screen().size();

    switch (state_) {
    case State::inactive:
        if (manhattan_length(game.get_player().get_position(), position_) <
            std::min(screen_size.x, screen_size.y) / 2) {
            state_ = State::idle;
        }
        break;

    case State::idle:
        if (timer_ >= 200000) {
            timer_ -= 200000;
            if (random_choice<2>()) {
                state_ = State::dash_begin;
                sprite_.set_texture_index(TextureMap::dasher_crouch);
            } else {
                state_ = State::shoot_begin;
                sprite_.set_texture_index(TextureMap::dasher_weapon1);
            }
        }
        break;

    case State::pause:
        if (timer_ >= 200000) {
            timer_ -= 200000;
            state_ = State::dash_begin;
            sprite_.set_texture_index(TextureMap::dasher_crouch);
        }
        break;

    case State::shoot_begin:
        face_player();
        if (timer_ > 80000) {
            timer_ -= 80000;
            state_ = State::shooting;
            sprite_.set_texture_index(TextureMap::dasher_weapon2);
        }
        break;

    case State::shooting:
        if (timer_ > 300000) {
            state_ = State::pause;
        }
        break;

    case State::dash_begin:
        face_player();
        if (timer_ > 352000) {
            timer_ -= 352000;
            state_ = State::dashing;
            sprite_.set_texture_index(TextureMap::dasher_dash);
        }
        break;

    case State::dashing:
        if (timer_ > 250000) {
            timer_ -= 250000;
            state_ = State::dash_end;
            sprite_.set_texture_index(TextureMap::dasher_crouch);
        }
        break;

    case State::dash_end:
        if (timer_ > 150000) {
            timer_ -= 150000;
            state_ = State::idle;
            sprite_.set_texture_index(TextureMap::dasher_idle);
        }
        break;
    }

    switch (sprite_.get_texture_index()) {
    case TextureMap::dasher_crouch:
        head_.set_position({position_.x, position_.y - 6});
        break;

    case TextureMap::dasher_idle:
        head_.set_position({position_.x, position_.y - 9});
        break;

    case TextureMap::dasher_dash:
        head_.set_position(
            {sprite_.get_flip().x ? position_.x + 3 : position_.x - 3,
             position_.y - 5});
        break;

    case TextureMap::dasher_weapon1:
        head_.set_position({position_.x, position_.y - 9});
        break;

    case TextureMap::dasher_weapon2:
        head_.set_position({position_.x, position_.y - 9});
        break;
    }
}
