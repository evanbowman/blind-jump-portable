#include "golem.hpp"
#include "game.hpp"
#include "common.hpp"
#include "wallCollision.hpp"


Golem::Golem(const Vec2<Float>& pos) :
    Enemy(Entity::Health(10), pos, {{20, 24}, {10, 14}}),
    state_(State::sleep),
    count_(0),
    timer_(0)
{
    sprite_.set_position(pos);
    sprite_.set_origin({16, 8});
    sprite_.set_texture_index(18);

    head_.set_position(pos);
    head_.set_origin({16, 40});
    head_.set_texture_index(17);

    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -9});
    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_position(pos);
}


void Golem::injured(Platform& pfrm, Game& game, Health amount)
{
    if (sprite_.get_mix().amount_ < 180) {
        pfrm.sleep(2);
    }

    debit_health(pfrm, amount);

    const auto c = current_zone(game).injury_glow_color_;

    if (alive()) {
        pfrm.speaker().play_sound("click", 1, position_);
    } else {
        const auto add_score = 20;

        game.score() += add_score;
    }

    sprite_.set_mix({c, 255});
    head_.set_mix({c, 255});

    if (state_ == State::inactive) {
        state_ = State::idle;
    }
}


void Golem::on_collision(Platform& pfrm, Game& game, AlliedOrbShot&)
{
    injured(pfrm, game, 1);
}


void Golem::on_collision(Platform& pfrm, Game& game, LaserExplosion&)
{
    injured(pfrm, game, 8);
}


void Golem::on_collision(Platform& pfrm, Game& game, Laser&)
{
    injured(pfrm, game, 1);
}


void Golem::update(Platform& pfrm, Game& game, Microseconds dt)
{
    fade_color_anim_.advance(sprite_, dt);
    head_.set_mix(sprite_.get_mix());

    static const float kickback_rate = 0.00005f;
    static const float movement_rate = 0.00004f;

    auto face_left = [this] {
        sprite_.set_flip({0, 0});
        head_.set_flip({0, 0});
    };

    auto face_right = [this] {
        sprite_.set_flip({1, 0});
        head_.set_flip({1, 0});
    };

    auto face_target = [&] {
        if (get_target(game).get_position().x > position_.x) {
            face_right();
        } else {
            face_left();
        }
    };

    auto check_wall = [&] {
        const auto wc = check_wall_collisions(game.tiles(), *this);
        if (wc.any()) {
            if ((wc.left and speed_.x < 0.f) or (wc.right and speed_.x > 0.f)) {
                speed_.x = 0.f;
            }
            if ((wc.up and speed_.y < 0.f) or (wc.down and speed_.y > 0.f)) {
                speed_.y = 0.f;
            }
        }

    };

    switch (state_) {
    case State::sleep:
        timer_ += dt;
        if (timer_ > seconds(2)) {
            timer_ = 0;
            state_ = State::inactive;
        }
        break;

    case State::inactive:
        if (visible() or pfrm.network_peer().is_connected()) {
            timer_ = 0;
            state_ = State::idle;
        }
        break;

    case State::idle:
        face_target();

        speed_ = direction(position_, get_target(game).get_position()) * 1.4f;
        check_wall();
        position_.x += speed_.x * dt * movement_rate;
        position_.y += speed_.y * dt * movement_rate;

        timer_ += dt;
        if (timer_ > milliseconds(1550)) {
            timer_ = 0;
            if (visible() and distance(position_, get_target(game).get_position()) < 140) {
                state_ = State::charge;
            }
        }
        break;

    case State::charge:
        face_target();
        if (timer_ > milliseconds(75) and
            timer_ + dt > milliseconds(75)) {
            target_ = get_target(game).get_position();
        }
        timer_ += dt;
        if (timer_ > milliseconds(150)) {
            timer_ = 0;
            state_ = State::shooting;
        }
        break;

    case State::shooting:
        timer_ += dt;

        target_ = interpolate(get_target(game).get_position(),
                              target_,
                              dt * 0.000012f);

        if (timer_ > milliseconds(70)) {
            timer_ = 0;

            pfrm.speaker().play_sound("laser1", 4, position_);
            this->shoot(
                pfrm,
                game,
                position_,
                rng::sample<24>(target_, rng::utility_state),
                0.00019f);

            speed_ = direction(target_, position_) * 1.4f;

            if (++count_ > 5) {
                count_ = 0;
                state_ = State::pause;
            }
        } else {
            speed_ = interpolate(Vec2<Float>{0, 0}, speed_, dt * 0.00005f);
        }
        check_wall();
        position_.x += speed_.x * dt * kickback_rate;
        position_.y += speed_.y * dt * kickback_rate;
        break;

    case State::pause:
        timer_ += dt;
        speed_ = interpolate(Vec2<Float>{0, 0}, speed_, dt * 0.00005f);
        check_wall();
        position_.x += speed_.x * dt * kickback_rate;
        position_.y += speed_.y * dt * kickback_rate;

        if (timer_ > seconds(1)) {
            timer_ = 0;
            state_ = State::idle;
            speed_ = {};
        }
        break;
    }

    sprite_.set_position(position_);
    head_.set_position(position_);
    shadow_.set_position(position_);
}


void Golem::on_death(Platform& pfrm, Game& game)
{
    pfrm.sleep(6);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::heart,
                                               Item::Type::null};

    on_enemy_destroyed(pfrm, game, 0, position_, 3, item_drop_vec);
}
