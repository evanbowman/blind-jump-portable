#include "theif.hpp"
#include "blind_jump/game.hpp"
#include "common.hpp"
#include "number/random.hpp"


Theif::Theif(const Vec2<Float>& pos)
    : Enemy(Entity::Health(2), pos, {{16, 32}, {8, 16}}), timer_(0),
      shadow_check_timer_(rng::choice<milliseconds(300)>(rng::utility_state)),
      float_amount_(0), float_timer_(0), tries_(0), state_(State::sleep)
{
    set_position(pos);

    sprite_.set_position(pos);
    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_texture_index(43);
    sprite_.set_origin({16, 16});

    shadow_.set_position(pos);
    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -9});
    shadow_.set_alpha(Sprite::Alpha::translucent);
}


void Theif::update(Platform& pfrm, Game& game, Microseconds dt)
{
    Enemy::update(pfrm, game, dt);

    fade_color_anim_.advance(sprite_, dt);

    auto face_left = [this] { sprite_.set_flip({0, 0}); };

    auto face_right = [this] { sprite_.set_flip({1, 0}); };

    if (visible()) {
        float_timer_ += dt;
        if (float_timer_ > milliseconds(500)) {
            if (float_amount_) {
                float_amount_ = 0;
            } else {
                float_amount_ = 1;
            }
            float_timer_ = 0;
        }
    }

    sprite_.set_position(Vec2<Float>{position_.x, position_.y - float_amount_});

    auto face_player =
        [this, &player = game.player(), &face_left, &face_right] {
            if (player.get_position().x > position_.x) {
                face_right();
            } else {
                face_left();
            }
        };

    switch (state_) {
    case State::inactive:
        if (visible()) {
            state_ = State::sleep;
        }
        break;

    case State::sleep:
        timer_ += dt;
        if (timer_ > seconds(2)) {
            timer_ = 0;
            state_ = State::idle;
            tries_ = 4;
        }
        break;

    case State::idle:
        timer_ += dt;

        face_player();

        if (timer_ > milliseconds(500)) {
            timer_ = 0;
            state_ = State::approach;
            sprite_.set_texture_index(44);
            auto target = rng::sample<16>(game.player().get_position(),
                                          rng::critical_state);
            step_vector_ = direction(position_, target) * 0.00013f;

            --tries_;

            if (step_vector_.x > 0) {
                face_right();
            } else {
                face_left();
            }
        }
        break;

    case State::approach:
        timer_ += dt;

        shadow_.set_position(position_);

        shadow_check_timer_ -= dt;
        if (shadow_check_timer_ < 0) {

            shadow_check_timer_ = milliseconds(100);

            auto coord = to_tile_coord(position_.cast<s32>());

            if (not is_walkable(game.tiles().get_tile(coord.x, coord.y))) {
                shadow_.set_alpha(Sprite::Alpha::transparent);
            } else {
                shadow_.set_alpha(Sprite::Alpha::translucent);
            }
        }

        if (timer_ > milliseconds(750)) {
            timer_ = 0;

            auto coord = to_tile_coord(position_.cast<s32>());

            if (not is_walkable(game.tiles().get_tile(coord.x, coord.y))) {
                shadow_.set_alpha(Sprite::Alpha::transparent);
            } else {
                shadow_.set_alpha(Sprite::Alpha::translucent);
            }

            if (tries_ == 0) {
                state_ = State::sleep;
            } else {
                state_ = State::idle;
            }
            sprite_.set_texture_index(43);
        }

        position_.x += step_vector_.x * dt;
        position_.y += step_vector_.y * dt;

        break;
    }
}


void Theif::on_collision(Platform&, Game& game, Player&)
{
    if (state_ == State::approach) {
        tries_ = 0;
    }
}


void Theif::on_collision(Platform& pfrm, Game& game, LaserExplosion&)
{
    injured(pfrm, game, 8);
}


void Theif::on_collision(Platform& pfrm, Game& game, AlliedOrbShot&)
{
    if (not is_allied()) {
        injured(pfrm, game, 1);
    }
}


void Theif::on_collision(Platform& pfrm, Game& game, Laser& laser)
{
    if (state_ == State::approach) {
        injured(pfrm, game, 1);
    }
}


void Theif::injured(Platform& pfrm, Game& game, Health amount)
{
    sprite_.set_mix({current_zone(game).injury_glow_color_, 255});

    debit_health(pfrm, amount);

    if (alive()) {
        pfrm.speaker().play_sound("click", 1, position_);
    } else {
        game.score() += 10;
    }
}


void Theif::on_death(Platform& pf, Game& game)
{
    pf.sleep(5);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::heart};

    on_enemy_destroyed(pf, game, 0, position_, 0, item_drop_vec);
}
