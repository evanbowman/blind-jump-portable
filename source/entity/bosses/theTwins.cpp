#include "theTwins.hpp"
#include "game.hpp"


static const Entity::Health initial_health = 60;


void Twin::set_sprite(TextureIndex index)
{
    sprite_.set_texture_index(index);
    head_.set_texture_index(index + 1);
}


void Twin::update_sprite()
{
    // Because we are using two adjacent horizontal sprites for this boss, we
    // need to transpose their positions if flipped, otherwise each constituent
    // sprite will be flipped in place
    if (head_.get_flip().x) {
        head_.set_position({position_.x - 18, position_.y});
        sprite_.set_position({position_.x + 14, position_.y});
        shadow_.set_position({position_.x + 2, position_.y});
        shadow2_.set_position({position_.x + 2, position_.y});
        hitbox_.dimension_.origin_ = {16, 8};
    } else {
        head_.set_position({position_.x + 18, position_.y});
        sprite_.set_position({position_.x - 14, position_.y});
        shadow_.set_position({position_.x - 2, position_.y});
        shadow2_.set_position({position_.x - 2, position_.y});
        hitbox_.dimension_.origin_ = {28, 8};
    }

}


Twin::Twin(const Vec2<Float>& position)
    : Enemy(initial_health, position, {{44, 16}, {0, 0}})
{
    head_.set_size(Sprite::Size::w32_h32);
    head_.set_origin({16, 16});

    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_origin({16, 16});

    shadow_.set_size(Sprite::Size::w32_h32);
    shadow_.set_origin({32, 9});
    shadow_.set_texture_index(33);
    shadow_.set_alpha(Sprite::Alpha::translucent);

    shadow2_.set_size(Sprite::Size::w32_h32);
    shadow2_.set_origin({0, 9});
    shadow2_.set_texture_index(34);
    shadow2_.set_alpha(Sprite::Alpha::translucent);

    set_sprite(6);
}


void Twin::update(Platform& pf, Game& game, Microseconds dt)
{
    auto face_left = [this] {
        head_.set_flip({0, 0});
        sprite_.set_flip({0, 0});
    };

    auto face_right = [this] {
        head_.set_flip({1, 0});
        sprite_.set_flip({1, 0});
    };

    auto& target = get_target(game);

    auto face_target = [this, &target, &face_left, &face_right] {
        if (target.get_position().x > position_.x) {
            face_right();
        } else {
            face_left();
        }
    };

    switch (state_) {
    case State::inactive:
        face_target();
        state_ = State::idle;
        break;

    case State::idle:
        timer_ += dt;
        if (timer_ > seconds(2)) {
            state_ = State::open_mouth;
            timer_ = 0;
        }
        break;

    case State::open_mouth:
        timer_ += dt;
        if (timer_ > milliseconds(100)) {
            timer_ = 0;
            auto index = sprite_.get_texture_index();
            if (index < 14) {
                set_sprite(index + 2);
            } else {
                state_ = State::ranged_attack_charge;

                // if (rng::choice<2>(rng::critical_state)) {
                //     timer_ = seconds(1);
                // }
            }
        }
        break;

    case State::ranged_attack_charge:
        if (timer_ < seconds(1) and timer_ + dt > seconds(1)) {
            sprite_.set_mix({current_zone(game).energy_glow_color_, 0});
        }
        timer_ += dt;
        alt_timer_ += dt;
        if (alt_timer_ > milliseconds(80)) {
            alt_timer_ = 0;
            if (sprite_.get_texture_index() == 14) {
                sprite_.set_texture_index(41);
                alt_timer_ = -milliseconds(30);
            } else {
                sprite_.set_texture_index(14);
            }
        }
        if (timer_ > seconds(2) - milliseconds(100)) {
            timer_ = 0;
            state_ = State::ranged_attack;
            sprite_.set_texture_index(14);
            medium_explosion(pf, game, sprite_.get_position());
            game.camera().shake();
        }
        break;

    case State::ranged_attack:
        timer_ += dt;
        if (timer_ > seconds(3)) {
            state_ = State::ranged_attack_done;
        }
        break;

    case State::ranged_attack_done:
        state_ = State::idle;
        set_sprite(6);
        timer_ = milliseconds(rng::choice<800>(rng::critical_state));
        break;
    }

    update_sprite();

    if (sprite_.get_mix().color_ == current_zone(game).energy_glow_color_) {
        fade_color_anim_.reverse(sprite_, dt);
    } else {
        fade_color_anim_.advance(sprite_, dt);
    }
    head_.set_mix(sprite_.get_mix());
}


void Twin::injured(Platform& pfrm, Game& game, Health amount)
{
    if (alive()) {
        pfrm.speaker().play_sound("click", 1, position_);
    }

    debit_health(pfrm, amount);

    const auto c = current_zone(game).injury_glow_color_;

    sprite_.set_mix({c, 255});
    head_.set_mix({c, 255});
}


void Twin::on_collision(Platform& pfrm, Game& game, LaserExplosion&)
{
    injured(pfrm, game, Health{8});
}


void Twin::on_collision(Platform& pfrm, Game& game, AlliedOrbShot&)
{
    injured(pfrm, game, Health{1});
}


void Twin::on_collision(Platform& pfrm, Game& game, Player&)
{
}


void Twin::on_collision(Platform& pfrm, Game& game, Laser&)
{
    injured(pfrm, game, Health{1});
}
