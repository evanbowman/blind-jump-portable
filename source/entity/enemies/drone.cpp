#include "drone.hpp"
#include "common.hpp"
#include "game.hpp"
#include "number/random.hpp"


Drone::Drone(const Vec2<Float>& pos)
    : Enemy(Entity::Health(4), pos, {{16, 16}, {8, 13}}), state_{State::sleep},
      timer_(0)
{
    sprite_.set_position(pos);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 13});
    sprite_.set_texture_index(TextureMap::drone);

    shadow_.set_position(pos);
    shadow_.set_origin({7, -9});
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_texture_index(TextureMap::drop_shadow);
}


void Drone::update(Platform& pfrm, Game& game, Microseconds dt)
{
    fade_color_anim_.advance(sprite_, dt);

    if (game.player().get_position().x > position_.x) {
        sprite_.set_flip({true, false});
    } else {
        sprite_.set_flip({false, false});
    }

    timer_ += dt;

    switch (state_) {
    case State::sleep:
        if (timer_ > seconds(2)) {
            timer_ = 0;
            state_ = State::inactive;
        }
        break;

    case State::inactive:
        if (visible()) {
            timer_ = 0;
            const auto screen_size = pfrm.screen().size();
            if (manhattan_length(game.player().get_position(), position_) <
                std::min(screen_size.x, screen_size.y)) {
                state_ = State::idle2;
            }
        }
        break;

    case State::idle1:
        if (timer_ > milliseconds(700)) {
            timer_ = 0;
            state_ = State::dodge1;
            const auto player_pos = game.player().get_position();
            step_vector_ =
                direction(position_, sample<64>(player_pos)) * 0.000055f;
        }
        break;

    case State::dodge1:
        position_ = position_ + Float(dt) * step_vector_;
        sprite_.set_position(position_);
        shadow_.set_position(position_);
        if (timer_ > seconds(1)) {
            timer_ = 0;
            state_ = State::idle2;
        }
        break;

    case State::idle2:
        if (timer_ > milliseconds(700)) {
            timer_ = 0;
            state_ = State::dodge2;
            const auto player_pos = game.player().get_position();
            step_vector_ =
                direction(position_, sample<64>(player_pos)) * 0.000055f;
        }
        break;

    case State::dodge2:
        position_ = position_ + Float(dt) * step_vector_;
        sprite_.set_position(position_);
        shadow_.set_position(position_);
        if (timer_ > seconds(1)) {
            timer_ = 0;
            state_ = State::idle3;
        }
        break;

    case State::idle3:
        if (timer_ > milliseconds(700)) {
            timer_ = 0;
            state_ = State::dodge3;
            const auto player_pos = game.player().get_position();
            step_vector_ =
                direction(position_, sample<64>(player_pos)) * 0.000055f;
        }
        break;

    case State::dodge3:
        position_ = position_ + Float(dt) * step_vector_;
        sprite_.set_position(position_);
        shadow_.set_position(position_);
        if (timer_ > seconds(1)) {
            timer_ = 0;
            state_ = State::idle4;
        }
        break;

    case State::idle4:
        if (timer_ > milliseconds(700)) {
            timer_ = 0;
            state_ = State::rush;
            const auto player_pos = game.player().get_position();
            step_vector_ = direction(position_, player_pos) * 0.00015f;
        }
        break;

    case State::rush:
        position_ = position_ + Float(dt) * step_vector_;
        sprite_.set_position(position_);
        shadow_.set_position(position_);
        if (timer_ > seconds(1)) {
            timer_ = 0;
            state_ = State::idle1;
        }
        break;
    }
}


void Drone::injured(Platform& pf, Game& game, Health amount)
{
    sprite_.set_mix({current_zone(game).injury_glow_color_, 255});

    debit_health(amount);

    if (alive()) {
        pf.speaker().play_sound("click", 1);
    }
}


void Drone::on_collision(Platform& pf, Game& game, Laser&)
{
    injured(pf, game, Health{1});
}


void Drone::on_collision(Platform& pf, Game& game, LaserExplosion&)
{
    injured(pf, game, Health{8});
}


void Drone::on_collision(Platform& pf, Game& game, Player& player)
{
    if (state_ == State::rush) {
        this->kill();
    }
}


void Drone::on_death(Platform& pf, Game& game)
{
    game.score() += 10;

    pf.sleep(5);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::null};

    on_enemy_destroyed(pf, game, position_, 7, item_drop_vec);
}
