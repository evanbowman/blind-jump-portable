#include "player.hpp"
#include "platform.hpp"


// NOTE: The player code was ported over from the original version of
// BlindJump from 2016. This code is kind of a mess.


Player::Player() :
    frame_(0),
    frame_base_(ResourceLoc::still_down),
    anim_timer_(0),
    l_speed_(0.f),
    r_speed_(0.f),
    u_speed_(0.f),
    d_speed_(0.f)
{
    sprite_.set_position({104.f, 64.f});
    shadow_.set_texture_index(33);
    shadow_.set_alpha(Sprite::Alpha::translucent);
}


template <Player::ResourceLoc L>
void Player::key_response(bool k1,
                          bool k2,
                          bool k3,
                          bool k4,
                          float& speed,
                          bool collision)
{
    if (k1) {
        if (not k2 and not k3 and not k4 and frame_base_ != L) {
            frame_base_ = L;
        }
        if (not collision) {
            if (k3 || k4) {
                speed = .170f;
            } else {
                speed = .220f;
            }
        } else {
            speed = 0.f;
        }
    } else {
        speed = 0.f;
    }
}


template <Player::ResourceLoc S, uint8_t maxIndx>
void Player::on_key_released(bool k2, bool k3, bool k4, bool x) {
    if (not k2 && not k3 && not k4) {
        if (not x) {
            frame_base_ = S;
            frame_ = maxIndx;
        } else {
            switch (frame_base_) {
            case Player::ResourceLoc::walk_down:
                frame_base_ = Player::ResourceLoc::still_down;
                frame_ = 0;
                break;

            case Player::ResourceLoc::walk_up:
                frame_base_ = Player::ResourceLoc::still_up;
                frame_ = 0;
                break;

            case Player::ResourceLoc::walk_left:
                frame_base_ = Player::ResourceLoc::still_left;
                frame_ = 0;
                break;

            case Player::ResourceLoc::walk_right:
                frame_base_ = Player::ResourceLoc::still_right;
                frame_ = 0;
                break;

            default:
                break;
            }
        }
    }
}


static uint8_t remap_vframe(uint8_t index) {
    switch (index) {
    case 0: return 1;
    case 1: return 2;
    case 2: return 2;
    case 3: return 1;
    case 4: return 0;
    case 5: return 3;
    case 6: return 4;
    case 7: return 4;
    case 8: return 3;
    case 9: return 0;
    default: return 0;
    }
}


void Player::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto& input = pfrm.keyboard();
    const bool up = input.pressed<Keyboard::Key::up>();
    const bool down = input.pressed<Keyboard::Key::down>();
    const bool left = input.pressed<Keyboard::Key::left>();
    const bool right = input.pressed<Keyboard::Key::right>();

    key_response<ResourceLoc::walk_up>(up, down, left, right, u_speed_, false);
    key_response<ResourceLoc::walk_down>(down, up, left, right, d_speed_, false);
    key_response<ResourceLoc::walk_left>(left, right, down, up, l_speed_, false);
    key_response<ResourceLoc::walk_right>(right, left, down, up, r_speed_, false);

    if (input.up_transition<Keyboard::Key::up>()) {
        on_key_released<ResourceLoc::still_up, 0>(down, left, right, false);
    }
    if (input.up_transition<Keyboard::Key::down>()) {
        on_key_released<ResourceLoc::still_down, 0>(up, left, right, false);
    }
    if (input.up_transition<Keyboard::Key::left>()) {
        on_key_released<ResourceLoc::still_left, 0>(up, down, right, false);
    }
    if (input.up_transition<Keyboard::Key::right>()) {
        on_key_released<ResourceLoc::still_right, 0>(up, down, left, false);
    }

    switch (frame_base_) {
    case ResourceLoc::still_up:
    case ResourceLoc::still_down:
    case ResourceLoc::still_left:
    case ResourceLoc::still_right:
        sprite_.set_texture_index(frame_base_);
        break;

    case ResourceLoc::walk_up:
        update_animation(dt, 9, 100000);
        sprite_.set_texture_index(frame_base_ + remap_vframe(frame_));
        break;

    case ResourceLoc::walk_down:
        update_animation(dt, 9, 100000);
        sprite_.set_texture_index(frame_base_ + remap_vframe(frame_));
        break;

    case ResourceLoc::walk_left:
        update_animation(dt, 5, 100000);
        sprite_.set_texture_index(frame_base_ + frame_);
        break;

    case ResourceLoc::walk_right:
        update_animation(dt, 5, 100000);
        sprite_.set_texture_index(frame_base_ + frame_);
        break;
    }

    const auto& position = sprite_.get_position();
    Vec2<Float> new_pos { position.x - (l_speed_ + -r_speed_),
                          position.y - (u_speed_ + -d_speed_) };
    sprite_.set_position(new_pos);
    shadow_.set_position({ new_pos.x + 8, new_pos.y + 25});
}


void Player::update_animation(Microseconds dt, u8 max_index, Microseconds count)
{
    anim_timer_ += dt;
    if (anim_timer_ > count) {
        anim_timer_ -= count;
        frame_ += 1;
    }
    if (frame_ > max_index) {
        frame_ = 0;
    }
}
