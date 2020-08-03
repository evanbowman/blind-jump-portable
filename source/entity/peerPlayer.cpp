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
}

#include <iostream>
void PeerPlayer::sync(const net_event::PlayerInfo& info)
{
    if (distance(position_, info.position_.cast<Float>()) > 6) {
        interp_offset_ = position_ - info.position_.cast<Float>();
        position_ = info.position_.cast<Float>();
        std::cout << interp_offset_.x << interp_offset_.y << std::endl;
    } else {
        position_ = info.position_.cast<Float>();
    }

    update_sprite_position();

    anim_timer_ = milliseconds(100);

    sprite_.set_texture_index(info.texture_index_);
    sprite_.set_size(info.size_);
    speed_ = info.speed_;

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

    switch (sprite_.get_size()) {
    case Sprite::Size::w16_h32:
        break;

    case Sprite::Size::w32_h32:
        if (texture_index >= player_walk_right and
            texture_index < player_walk_right + 5) {

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
        if (texture_index >= player_walk_left and
            texture_index < player_walk_left + 5) {

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


    Vec2<Float> new_pos{
        position_.x - ((speed_.x) * dt * MOVEMENT_RATE_CONSTANT),
        position_.y - ((speed_.y) * dt * MOVEMENT_RATE_CONSTANT)};

    interp_offset_ =
        interpolate(Vec2<Float>{0.f, 0.f}, interp_offset_, dt * 0.00004f);

    set_position(new_pos);

    update_sprite_position();
}
