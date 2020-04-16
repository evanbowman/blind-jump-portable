#include "dasher.hpp"
#include "common.hpp"
#include "game.hpp"
#include "number/random.hpp"
#include "wallCollision.hpp"


Dasher::Dasher(const Vec2<Float>& position)
    : Enemy(Entity::Health(6), position, {{16, 32}, {8, 16}}), timer_(0),
      state_(State::sleep)
{
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

    auto face_left = [this] {
        sprite_.set_flip({0, 0});
        head_.set_flip({0, 0});
    };

    auto face_right = [this] {
        sprite_.set_flip({1, 0});
        head_.set_flip({1, 0});
    };

    auto face_player =
        [this, &player = game.player(), &face_left, &face_right] {
            if (player.get_position().x > position_.x) {
                face_right();
            } else {
                face_left();
            }
        };

    timer_ += dt;


    fade_color_anim_.advance(sprite_, dt);
    head_.set_mix(sprite_.get_mix());


    const auto& screen_size = pf.screen().size();

    switch (state_) {
    case State::sleep:
        if (timer_ > seconds(2)) {
            timer_ = 0;
            state_ = State::inactive;
        }
        break;

    case State::inactive: {
        if (visible()) {
            timer_ = 0;
            if (manhattan_length(game.player().get_position(), position_) <
                std::min(screen_size.x, screen_size.y)) {
                state_ = State::idle;
            }
        }
        break;
    }

    case State::idle:
        if (timer_ >= milliseconds(200)) {
            timer_ -= milliseconds(200);

            if (manhattan_length(game.player().get_position(), position_) <
                    std::min(screen_size.x, screen_size.y) and
                random_choice<2>()) {
                state_ = State::shoot_begin;
                sprite_.set_texture_index(TextureMap::dasher_weapon1);
            } else {
                state_ = State::dash_begin;
                sprite_.set_texture_index(TextureMap::dasher_crouch);
            }
        }
        break;

    case State::pause:
        if (timer_ >= milliseconds(200)) {
            timer_ -= milliseconds(200);
            state_ = State::dash_begin;
            sprite_.set_texture_index(TextureMap::dasher_crouch);
        }
        break;

    case State::shoot_begin:
        face_player();
        if (timer_ > milliseconds(80)) {
            timer_ -= milliseconds(80);
            state_ = State::shot1;
            sprite_.set_texture_index(TextureMap::dasher_weapon2);
        }
        break;

    case State::shot1:
        if (timer_ > milliseconds(50)) {
            timer_ -= milliseconds(50);
            state_ = State::shot2;

            game.effects().spawn<OrbShot>(
                position_, sample<8>(game.player().get_position()), 0.00015f);
        }
        break;

    case State::shot2:
        if (timer_ > milliseconds(150)) {
            timer_ -= milliseconds(150);
            state_ = State::shot3;

            game.effects().spawn<OrbShot>(
                position_, sample<16>(game.player().get_position()), 0.00015f);
        }
        break;

    case State::shot3:
        if (timer_ > milliseconds(150)) {
            timer_ -= milliseconds(150);
            state_ = State::pause;

            game.effects().spawn<OrbShot>(
                position_, sample<32>(game.player().get_position()), 0.00015f);
        }
        break;

    case State::dash_begin:
        if (timer_ > milliseconds(352)) {
            timer_ -= milliseconds(352);

            s16 dir =
                ((static_cast<float>(random_choice<359>())) / 360) * INT16_MAX;

            speed_.x = 5 * (float(cosine(dir)) / INT16_MAX);
            speed_.y = 5 * (float(sine(dir)) / INT16_MAX);
            state_ = State::dashing;
            sprite_.set_texture_index(TextureMap::dasher_dash);
        }
        break;

    case State::dashing: {
        auto next_state = [this] {
            state_ = State::dash_end;
            sprite_.set_texture_index(TextureMap::dasher_crouch);
        };
        if (timer_ > milliseconds(200)) {
            timer_ -= milliseconds(200);
            next_state();
        }
        const auto wc = check_wall_collisions(game.tiles(), *this);
        if (wc.any()) {
            if ((wc.left and speed_.x < 0.f) or (wc.right and speed_.x > 0.f)) {
                speed_.x = 0.f;
            }
            if ((wc.up and speed_.y < 0.f) or (wc.down and speed_.y > 0.f)) {
                speed_.y = 0.f;
            }
            if (speed_.x == 0.f and speed_.y == 0.f) {
                timer_ = 0;
                next_state();
            }
        }

        if (speed_.x < 0) {
            face_left();
        } else {
            face_right();
        }
        static const float movement_rate = 0.00005f;
        position_.x += speed_.x * dt * movement_rate;
        position_.y += speed_.y * dt * movement_rate;
    } break;

    case State::dash_end:
        if (timer_ > milliseconds(150)) {
            timer_ -= milliseconds(150);
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


void Dasher::on_collision(Platform& pf, Game& game, Laser&)
{
    debit_health(1);

    sprite_.set_mix({ColorConstant::aerospace_orange, 255});
    head_.set_mix({ColorConstant::aerospace_orange, 255});

    if (state_ == State::sleep) {
        timer_ = 0;
        state_ = State::shoot_begin;
    }
}


void Dasher::on_death(Platform& pf, Game& game)
{
    game.score() += 15;

    pf.sleep(5);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::coin,
                                               Item::Type::heart,
                                               Item::Type::null};

    on_enemy_destroyed(pf, game, position_, 2, item_drop_vec);


    if (random_choice<3>() == 0) {
        game.details().spawn<Item>(position_, pf, Item::Type::coin);
    }
}
