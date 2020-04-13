#include "firstExplorer.hpp"
#include "game.hpp"


FirstExplorer::FirstExplorer(const Vec2<Float>& position) :
    Entity(100), hitbox_{&position_, {16, 38}, {8, 24}}
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
            if (timer_ > seconds(2)) {
                state_ = State::draw_weapon;
                timer_ = 0;
            }
        }
        break;

    case State::still: {
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(200)) {
            timer_ = 0;
            state_ = State::draw_weapon;

            to_wide_sprite();

            sprite_.set_texture_index(7);
            head_.set_texture_index(41);
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
                head_.set_texture_index(std::min(u32(45), head_.get_texture_index() + 1));

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
            state_ = State::still;

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
}
