#include "firstExplorer.hpp"
#include "game.hpp"
#include "number/random.hpp"


static const char* boss_music = "sb_omega";


FirstExplorer::FirstExplorer(const Vec2<Float>& position)
    : Entity(100), hitbox_{&position_, {16, 38}, {8, 24}}, timer_(0), timer2_(0)
{
    set_position(position);

    sprite_.set_texture_index(12);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});

    head_.set_texture_index(13);
    head_.set_size(Sprite::Size::w16_h32);
    head_.set_origin({8, 32});

    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -9});
    shadow_.set_alpha(Sprite::Alpha::translucent);

    sprite_.set_position(position);
    head_.set_position({position_.x, position_.y - 16});
    shadow_.set_position(position);
}


void FirstExplorer::update(Platform& pf, Game& game, Microseconds dt)
{
    auto face_left = [this] {
        sprite_.set_flip({1, 0});
        head_.set_flip({1, 0});
    };

    auto face_right = [this] {
        sprite_.set_flip({0, 0});
        head_.set_flip({0, 0});
    };

    auto face_player =
        [this, &player = game.player(), &face_left, &face_right] {
            if (player.get_position().x > position_.x) {
                face_right();
            } else {
                face_left();
            }
        };

    auto to_wide_sprite = [&] {
        sprite_.set_size(Sprite::Size::w32_h32);
        sprite_.set_origin({16, 16});
        head_.set_size(Sprite::Size::w32_h32);
        head_.set_origin({16, 32});
    };

    auto to_narrow_sprite = [&] {
        sprite_.set_size(Sprite::Size::w16_h32);
        sprite_.set_origin({8, 16});
        head_.set_size(Sprite::Size::w16_h32);
        head_.set_origin({8, 32});
    };

    switch (state_) {
    case State::sleep:
        if (visible()) {
            timer_ += dt;
            if (timer_ > seconds(1)) {
                state_ = State::draw_weapon;
                timer_ = 0;

                game.play_music(pf, boss_music, seconds(235));
            }
        }
        break;

    case State::still: {
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(200)) {
            timer_ = 0;

            if (random_choice<2>()) {
                state_ = State::draw_weapon;

                to_wide_sprite();

                sprite_.set_texture_index(7);
                head_.set_texture_index(41);

            } else {
                state_ = State::prep_dash;
            }
        }
        break;
    }

    case State::draw_weapon: {
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(90)) {
            timer_ = 0;

            const auto index = sprite_.get_texture_index();
            if (index < 12) {
                sprite_.set_texture_index(index + 1);
                head_.set_texture_index(
                    std::min(u32(45), head_.get_texture_index() + 1));

            } else {
                state_ = State::shooting;
                timer_ = 0;
            }
        }

        break;
    }

    case State::shooting:
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(2000)) {
            timer_ = 0;
            state_ = State::prep_dash;

            to_narrow_sprite();

            sprite_.set_texture_index(12);
            head_.set_texture_index(13);
        }

        break;

    case State::prep_dash:
        timer_ += dt;
        if (timer_ > milliseconds(200)) {
            timer_ = 0;
            timer2_ = 0;

            to_wide_sprite();

            sprite_.set_texture_index(13);
            head_.set_texture_index(46);

            state_ = State::dash;
        }
        break;

    case State::dash:
        timer_ += dt;
        timer2_ += dt;

        if (timer_ > milliseconds(100)) {
            timer_ = 0;

            if (sprite_.get_texture_index() == 13) {
                sprite_.set_texture_index(14);
                head_.set_texture_index(47);
            } else {
                sprite_.set_texture_index(13);
                head_.set_texture_index(46);
            }
        }

        if (timer2_ > milliseconds(800)) {
            state_ = State::still;

            timer_ = 0;
            timer2_ = 0;

            to_narrow_sprite();

            sprite_.set_texture_index(12);
            head_.set_texture_index(13);
        }

        break;
    }

    fade_color_anim_.advance(sprite_, dt);
    head_.set_mix(sprite_.get_mix());
}


void FirstExplorer::on_collision(Platform& pf, Game& game, Laser&)
{
    debit_health(1);

    sprite_.set_mix({ColorConstant::aerospace_orange, 255});
    head_.set_mix({ColorConstant::aerospace_orange, 255});

    if (not alive()) {
        game.stop_music(pf);
    }
}
