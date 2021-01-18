#include "peerPlayer.hpp"
#include "blind_jump/game.hpp"
#include "wallCollision.hpp"


PeerPlayer::PeerPlayer(Platform& pfrm)
    : dynamic_texture_(pfrm.make_dynamic_texture())
{
    shadow_.set_origin({8, -9});
    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_alpha(Sprite::Alpha::translucent);
    interp_offset_ = {0.f, 0.f};
    blaster_.set_origin({8, 8});
    blaster_.set_size(Sprite::Size::w16_h32);
    blaster_.set_texture_index(h_blaster);

    head_.set_texture_index(120);
    head_.set_size(Sprite::Size::w16_h32);
}


void PeerPlayer::sync(Platform& pfrm,
                      Game& game,
                      const net_event::PlayerInfo& info)
{
    if (warping_) {
        return;
    }

    Vec2<Float> new_pos{static_cast<Float>(info.x_.get()),
                        static_cast<Float>(info.y_.get())};

    if (distance(position_, new_pos) > 6) {
        interp_offset_ = position_ - new_pos;
        position_ = new_pos;
    } else {
        position_ = new_pos;
    }

    if (info.get_visible()) {
        head_.set_alpha(Sprite::Alpha::opaque);
        sprite_.set_alpha(Sprite::Alpha::opaque);
        shadow_.set_alpha(Sprite::Alpha::translucent);
    } else {
        head_.set_alpha(Sprite::Alpha::transparent);
        sprite_.set_alpha(Sprite::Alpha::transparent);
        shadow_.set_alpha(Sprite::Alpha::transparent);
    }

    if (info.get_weapon_drawn()) {
        blaster_.set_alpha(Sprite::Alpha::opaque);
    } else {
        blaster_.set_alpha(Sprite::Alpha::transparent);
    }

    switch (info.get_display_color()) {
    case net_event::PlayerInfo::DisplayColor::none:
        sprite_.set_mix({});
        break;

    case net_event::PlayerInfo::DisplayColor::injured:
        sprite_.set_mix({current_zone(game).injury_glow_color_,
                         static_cast<u8>(info.get_color_amount())});
        break;

    case net_event::PlayerInfo::DisplayColor::got_coin:
        sprite_.set_mix({current_zone(game).energy_glow_color_,
                         static_cast<u8>(info.get_color_amount())});
        break;

    case net_event::PlayerInfo::DisplayColor::got_heart:
        sprite_.set_mix({ColorConstant::spanish_crimson,
                         static_cast<u8>(info.get_color_amount())});
        break;
    }

    head_.set_mix(sprite_.get_mix());
    blaster_.set_mix(sprite_.get_mix());

    if (info.get_texture_index() not_eq sprite_.get_texture_index()) {
        if (not dynamic_texture_) {
            dynamic_texture_ = pfrm.make_dynamic_texture();
        }

        if (dynamic_texture_) {
            (*dynamic_texture_)->remap(info.get_texture_index() * 2);
        }
    }
    sprite_.set_texture_index(info.get_texture_index());

    // Sorry about this switch statement. Basically, we want to render a
    // different colored helmet for the second player in multiplayer
    // modes. Given the limited size of available sprite memory on the gameboy
    // advance, we're drawing another sprite overtop of the player's head. But
    // because the player moves up and down by one pixel in some keyframes of
    // the walk cycle, we need to adjust the helmet's origin based on the
    // current keyframe.
    switch (sprite_.get_texture_index()) {
    case player_dodge_left:
        head_.set_texture_index(122);
        head_.set_origin({14, 38});
        break;

    case player_dodge_right:
        head_.set_texture_index(123);
        head_.set_origin({4, 38});
        break;

    case player_dodge_down:
        head_.set_texture_index(120);
        head_.set_origin({8, 36});
        break;

    case player_dodge_up:
        break;

    case player_sidestep_right:
        head_.set_texture_index(122);
        head_.set_origin({7, 40});
        break;

    case player_sidestep_left:
        head_.set_texture_index(123);
        head_.set_origin({9, 40});
        break;

    case player_still_down:
    case player_still_down + 3:
    case player_still_down + 5:
        head_.set_texture_index(120);
        head_.set_origin({8, 40});
        break;

    case player_still_down + 1:
    case player_still_down + 2:
    case player_still_down + 4:
        head_.set_texture_index(120);
        head_.set_origin({8, 39});
        break;

    case player_walk_up:
    case player_walk_up + 1:
    case player_walk_up + 3:
        head_.set_texture_index(121);
        head_.set_origin({8, 39});
        break;

    case player_walk_up + 2:
    case player_walk_up + 4:
    case player_walk_up + 5:
        head_.set_texture_index(121);
        head_.set_origin({8, 40});
        break;

    case player_walk_left:
    case player_walk_left + 1:
    case player_walk_left + 5:
        head_.set_texture_index(122);
        head_.set_origin({10, 39});
        break;

    case player_still_left:
        head_.set_texture_index(122);
        head_.set_origin({9, 40});
        break;

    case player_walk_left + 2:
    case player_walk_left + 3:
    case player_walk_left + 4:
        head_.set_texture_index(122);
        head_.set_origin({10, 40});
        break;

    case player_walk_right:
    case player_walk_right + 1:
    case player_walk_right + 5:
        head_.set_texture_index(123);
        head_.set_origin({6, 39});
        break;

    case player_still_right:
        head_.set_texture_index(123);
        head_.set_origin({7, 40});
        break;

    case player_walk_right + 2:
    case player_walk_right + 3:
    case player_walk_right + 4:
        head_.set_texture_index(123);
        head_.set_origin({6, 40});
        break;
    }

    sprite_.set_size(Sprite::Size::w32_h32);
    speed_.x = Float(info.x_speed_) / 10;
    speed_.y = Float(info.y_speed_) / 10;

    update_sprite_position();

    switch (info.get_sprite_size()) {
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
    // Note: head has origin shifted, with corresponding adjustment here. This
    // is a draw order hack, because the head sprite technically has a lower y
    // value, so it is drawn behind the player. Therefore, we've increased the
    // y-offset of the head sprite, and adjusted the head origin by the same
    // amount, to compensate.
    head_.set_position(Vec2<Float>{position_.x, position_.y + 24} +
                       interp_offset_);
    sprite_.set_position(position_ + interp_offset_);
    shadow_.set_position(position_ + interp_offset_);
}


void PeerPlayer::update(Platform& pfrm, Game& game, Microseconds dt)
{
    if (warping_) {
        head_.set_alpha(Sprite::Alpha::transparent);
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
    if (texture_index == player_dodge_down) {
        set_blaster_down();
    }

    if (texture_index == player_still_right) {
        set_blaster_right();
    }
    if (texture_index >= player_walk_right and
        texture_index < player_walk_right + 6) {

        set_blaster_right();
    }
    if (texture_index == player_dodge_right or
        texture_index == player_sidestep_right) {
        set_blaster_right();
    }

    if (texture_index == player_still_left) {
        set_blaster_left();
    }
    if (texture_index >= player_walk_left and
        texture_index < player_walk_left + 6) {

        set_blaster_left();
    }
    if (texture_index == player_dodge_left or
        texture_index == player_sidestep_left) {
        set_blaster_left();
    }
}
