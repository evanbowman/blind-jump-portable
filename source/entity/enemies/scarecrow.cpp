#include "scarecrow.hpp"
#include "common.hpp"
#include "game.hpp"
#include "number/random.hpp"


Scarecrow::Scarecrow(const Vec2<Float>& position)
    : Entity(3), hitbox_{&position_, {16, 32}, {8, 16}}
{
    position_ = {position.x, position.y - 32};

    sprite_.set_texture_index(TextureMap::scarecrow_top);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    sprite_.set_position(position_);

    leg_.set_texture_index(TextureMap::scarecrow_bottom);
    leg_.set_size(Sprite::Size::w16_h32);
    leg_.set_origin({8, -16});
    leg_.set_position(position_);

    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -32});
    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_position(position_);

    anchor_ = to_tile_coord(position.cast<s32>());
}


void Scarecrow::update(Platform& pfrm, Game& game, Microseconds dt)
{
    auto face_left = [this] {
        sprite_.set_flip({0, 0});
        leg_.set_flip({0, 0});
    };

    auto face_right = [this] {
        sprite_.set_flip({1, 0});
        leg_.set_flip({1, 0});
    };

    auto face_player =
        [this, &player = game.player(), &face_left, &face_right] {
            if (player.get_position().x > position_.x) {
                face_right();
            } else {
                face_left();
            }
        };

    face_player();

    fade_color_anim_.advance(sprite_, dt);
    leg_.set_mix(sprite_.get_mix());


    switch (state_) {
    case State::sleep:
        if (visible()) {
            state_ = State::idle_wait;
        }
        break;

    case State::idle_crouch: {
        const auto sprite_pos = sprite_.get_position();
        timer_ += dt;
        if (timer_ > milliseconds(35)) {
            timer_ = 0;
            if (sprite_pos.y < position_.y + 3) {
                sprite_.set_position({sprite_pos.x, sprite_pos.y + 1});
            } else {
                state_ = State::idle_wait;
            }
        }
        break;
    }

    case State::idle_wait:
        timer_ += dt;
        if (timer_ > milliseconds(100)) {
            timer_ = 0;
            sprite_.set_texture_index(TextureMap::scarecrow_top_2);
            state_ = State::idle_jump;
        }
        break;

    case State::idle_jump: {
        const auto sprite_pos = sprite_.get_position();
        timer_ += dt;
        if (timer_ > milliseconds(12)) {
            timer_ = 0;
            if (sprite_pos.y > position_.y + 1) {
                sprite_.set_position({sprite_pos.x, sprite_pos.y - 1});
            } else {
                sprite_.set_position(position_);
                state_ = State::idle_airborne;
            }
        }

        move_vec_ = direction(position_, sample<32>(to_world_coord(anchor_))) *
                    0.000005f;

        break;
    }

    case State::idle_airborne:
        timer_ += dt;
        if (visible()) {

            position_ = position_ + Float(dt) * move_vec_;

            const Float offset =
                8 * float(sine(4 * 3.14 * 0.0027f * timer_ + 180)) /
                std::numeric_limits<s16>::max();


            auto pos = sprite_.get_position();
            if (pos.y < position_.y - abs(offset) or abs(offset) > 7) {
                sprite_.set_texture_index(TextureMap::scarecrow_top);

                if (int(offset) == 0) {
                    state_ = State::idle_crouch;
                    timer_ = 0;
                }
            }

            sprite_.set_position({position_.x, position_.y - abs(offset)});

            leg_.set_position({position_.x, position_.y - abs(offset)});
            shadow_.set_position(position_);
        }
        break;

    case State::long_crouch:
        break;

    case State::long_wait:
        break;

    case State::long_jump:
        break;

    case State::long_airborne:
        break;
    }
}


void Scarecrow::on_collision(Platform& pf, Game& game, Laser&)
{
    debit_health(1);

    sprite_.set_mix({ColorConstant::aerospace_orange, 255});
    leg_.set_mix({ColorConstant::aerospace_orange, 255});

    if (not alive()) {
        game.score() += 15;

        pf.sleep(5);

        static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                                   Item::Type::coin,
                                                   Item::Type::coin,
                                                   Item::Type::heart,
                                                   Item::Type::null};

        on_enemy_destroyed(pf, game, position_, 3, item_drop_vec);


        if (random_choice<3>() == 0) {
            game.details().spawn<Item>(position_, pf, Item::Type::coin);
        }
    }
}
