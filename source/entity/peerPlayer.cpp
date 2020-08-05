#include "peerPlayer.hpp"
#include "game.hpp"
#include "wallCollision.hpp"


PeerPlayer::PeerPlayer()
{
    shadow_.set_origin({8, -9});
    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_alpha(Sprite::Alpha::translucent);
    interp_offset_ = {0.f, 0.f};
    blaster_.set_origin({8, 8});
    blaster_.set_size(Sprite::Size::w16_h32);
    blaster_.set_texture_index(h_blaster);
}


void PeerPlayer::sync(Game& game, const net_event::PlayerInfo& info)
{
    if (warping_) {
        return;
    }

    Vec2<Float> new_pos{static_cast<Float>(info.x_),
                        static_cast<Float>(info.y_)};

    if (distance(position_, new_pos) > 6) {
        interp_offset_ = position_ - new_pos;
        position_ = new_pos;
    } else {
        position_ = new_pos;
    }

    if (info.visible_) {
        sprite_.set_alpha(Sprite::Alpha::opaque);
        shadow_.set_alpha(Sprite::Alpha::translucent);
    } else {
        sprite_.set_alpha(Sprite::Alpha::transparent);
        shadow_.set_alpha(Sprite::Alpha::transparent);
    }

    if (info.weapon_drawn_) {
        blaster_.set_alpha(Sprite::Alpha::opaque);
    } else {
        blaster_.set_alpha(Sprite::Alpha::transparent);
    }

    switch (info.disp_color_) {
    case net_event::PlayerInfo::DisplayColor::none:
        sprite_.set_mix({});
        break;

    case net_event::PlayerInfo::DisplayColor::injured:
        sprite_.set_mix({current_zone(game).injury_glow_color_,
                         static_cast<u8>(info.color_amount_ << 4)});
        break;

    case net_event::PlayerInfo::DisplayColor::got_coin:
        sprite_.set_mix({current_zone(game).energy_glow_color_,
                         static_cast<u8>(info.color_amount_ << 4)});
        break;

    case net_event::PlayerInfo::DisplayColor::got_heart:
        sprite_.set_mix({ColorConstant::spanish_crimson,
                         static_cast<u8>(info.color_amount_ << 4)});
        break;
    }

    blaster_.set_mix(sprite_.get_mix());

    update_sprite_position();

    anim_timer_ = milliseconds(100);

    sprite_.set_texture_index(info.texture_index_);
    sprite_.set_size(info.size_);
    speed_.x = Float(info.x_speed_) / 10;
    speed_.y = Float(info.y_speed_) / 10;

    switch (info.size_) {
    case Sprite::Size::w16_h32:
        sprite_.set_origin({8, 16});
        break;


    case Sprite::Size::w32_h32:
        sprite_.set_origin({16, 16});
        break;
    }
}


void PeerPlayer::update_sprite_position()
{
    sprite_.set_position(position_ + interp_offset_);
    shadow_.set_position(position_ + interp_offset_);
}


void PeerPlayer::update(Platform& pfrm, Game& game, Microseconds dt)
{
    if (warping_) {
        sprite_.set_alpha(Sprite::Alpha::transparent);
        shadow_.set_alpha(Sprite::Alpha::transparent);
    }

    // Intentionally moves a bit slower than our player character. This actually reduces choppiness.
    static const float MOVEMENT_RATE_CONSTANT = 0.000044f;

    const auto wc = check_wall_collisions(game.tiles(), *this);

    if (wc.up and speed_.y < 0) {
        speed_.y = 0;
    }

    if (wc.down and speed_.y > 0) {
        speed_.y = 0;
    }

    if (wc.right and speed_.x > 0) {
        speed_.x = 0;
    }

    if (wc.left and speed_.x < 0) {
        speed_.x = 0;
    }

    auto texture_index = sprite_.get_texture_index();

    Vec2<Float> new_pos{
        position_.x - ((speed_.x) * dt * MOVEMENT_RATE_CONSTANT),
        position_.y - ((speed_.y) * dt * MOVEMENT_RATE_CONSTANT)};

    interp_offset_ =
        interpolate(Vec2<Float>{0.f, 0.f}, interp_offset_, dt * 0.00004f);

    set_position(new_pos);

    update_sprite_position();

    auto set_blaster_left = [&] {
        auto pos = position_ + interp_offset_;
        blaster_.set_texture_index(h_blaster);
        blaster_.set_flip({true, false});
        blaster_.set_position({pos.x - 12, pos.y + 1});
    };

    auto set_blaster_right = [&] {
        auto pos = position_ + interp_offset_;
        blaster_.set_texture_index(h_blaster);
        blaster_.set_flip({false, false});
        blaster_.set_position({pos.x + 12, pos.y + 1});
    };

    auto set_blaster_down = [&] {
        auto pos = position_ + interp_offset_;
        blaster_.set_texture_index(v_blaster);
        blaster_.set_flip({false, false});
        blaster_.set_position({pos.x - 3, pos.y + 1});
    };


    switch (sprite_.get_size()) {
    case Sprite::Size::w16_h32:
        if (texture_index >= player_walk_up and
            texture_index < player_walk_up + 5) {
        }
        if (texture_index == player_still_down) {
            set_blaster_down();
        }
        if (texture_index >= player_walk_down and
            texture_index < player_walk_down + 5) {

            set_blaster_down();
        }
        break;

    case Sprite::Size::w32_h32:
        if (texture_index == player_still_right) {
            set_blaster_right();
        }
        if (texture_index >= player_walk_right and
            texture_index < player_walk_right + 6) {

            set_blaster_right();

            anim_timer_ -= dt;
            if (anim_timer_ <= 0) {
                anim_timer_ = milliseconds(100);

                texture_index += 1;
                if (texture_index == player_walk_right + 5) {
                    texture_index = player_walk_right;
                }
                sprite_.set_texture_index(texture_index);
            }
        }
        if (texture_index == player_still_left) {
            set_blaster_left();
        }
        if (texture_index >= player_walk_left and
            texture_index < player_walk_left + 6) {

            set_blaster_left();

            anim_timer_ -= dt;
            if (anim_timer_ <= 0) {
                anim_timer_ = milliseconds(100);

                texture_index += 1;
                if (texture_index == player_walk_left + 5) {
                    texture_index = player_walk_left;
                }
                sprite_.set_texture_index(texture_index);
            }
        }
        break;
    }
}
