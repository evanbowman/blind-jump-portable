#include "drone.hpp"
#include "blind_jump/game.hpp"
#include "blind_jump/network_event.hpp"
#include "common.hpp"
#include "number/random.hpp"


Drone::Drone(const Vec2<Float>& pos)
    : Enemy(Entity::Health(4), pos, {{16, 16}, {8, 13}}), state_{State::sleep},
      timer_(0),
      shadow_check_timer_(milliseconds(500) +
                          rng::choice<milliseconds(300)>(rng::utility_state))
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
    Enemy::update(pfrm, game, dt);

    fade_color_anim_.advance(sprite_, dt);

    auto& target = get_target(game);

    if (target.get_position().x > position_.x) {
        sprite_.set_flip({true, false});
    } else {
        sprite_.set_flip({false, false});
    }

    timer_ += dt;

    auto send_state = [&] {
        if (&target == &game.player()) {
            const auto int_pos = position_.cast<s16>();

            net_event::EnemyStateSync s;
            s.state_ = static_cast<u8>(state_);
            s.x_.set(int_pos.x);
            s.y_.set(int_pos.y);
            s.id_.set(id());

            net_event::transmit(pfrm, s);
        }
    };

    if (visible()) {
        // Calculating our current tile coordinate is costly, but we do want to hide
        // our shadow from view when floating over empty space. So run the check
        // every so often.
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
    }

    switch (state_) {
    case State::sleep:
        if (timer_ > seconds(2)) {
            timer_ = 0;
            state_ = State::inactive;

            if (game.difficulty() == Settings::Difficulty::hard) {
                add_health(1);
            } else if (game.difficulty() == Settings::Difficulty::easy) {
                set_health(get_health() - 1);
            }
        }
        break;

    case State::inactive: {
        timer_ = 0;
        const auto screen_size = pfrm.screen().size();
        if (manhattan_length(target.get_position(), position_) <
            std::min(screen_size.x, screen_size.y)) {
            state_ = State::idle1;
        }
        break;
    }

    case State::idle1:
        if (timer_ > milliseconds(700)) {
            timer_ = 0;
            state_ = State::dodge1;
            const auto player_pos = target.get_position();
            step_vector_ =
                direction(position_,
                          rng::sample<64>(player_pos, rng::critical_state)) *
                0.000055f;
        }
        break;

    case State::dodge1:
        position_ = position_ + Float(dt) * step_vector_;
        sprite_.set_position(position_);
        shadow_.set_position(position_);
        if (timer_ > milliseconds(900)) {
            step_vector_ =
                interpolate(Vec2<Float>{0, 0}, step_vector_, dt * 0.000008f);
        }
        if (timer_ > seconds(1)) {
            timer_ = 0;
            state_ = State::idle2;

            send_state();
        }
        break;

    case State::idle2:
        if (timer_ > milliseconds(700)) {
            timer_ = 0;
            state_ = State::dodge2;
            const auto player_pos = target.get_position();
            step_vector_ =
                direction(position_,
                          rng::sample<64>(player_pos, rng::critical_state)) *
                0.000055f;
        }
        break;

    case State::dodge2:
        position_ = position_ + Float(dt) * step_vector_;
        sprite_.set_position(position_);
        shadow_.set_position(position_);
        if (timer_ > milliseconds(900)) {
            step_vector_ =
                interpolate(Vec2<Float>{0, 0}, step_vector_, dt * 0.000008f);
        }
        if (timer_ > seconds(1)) {
            timer_ = 0;
            state_ = State::idle3;

            send_state();
        }
        break;

    case State::idle3:
        if (timer_ > milliseconds(700)) {
            timer_ = 0;
            state_ = State::dodge3;
            const auto player_pos = target.get_position();
            step_vector_ =
                direction(position_,
                          rng::sample<64>(player_pos, rng::critical_state)) *
                0.000055f;
        }
        break;

    case State::dodge3:
        position_ = position_ + Float(dt) * step_vector_;
        sprite_.set_position(position_);
        shadow_.set_position(position_);
        if (timer_ > milliseconds(900)) {
            step_vector_ =
                interpolate(Vec2<Float>{0, 0}, step_vector_, dt * 0.000008f);
        }
        if (timer_ > seconds(1)) {
            timer_ = 0;
            state_ = State::idle4;

            send_state();
        }
        break;

    case State::idle4:
        if (timer_ > milliseconds(700)) {
            timer_ = 0;
            state_ = State::rush;
            const auto player_pos = target.get_position();
            step_vector_ = direction(position_, player_pos) *
                           (game.difficulty() not_eq Settings::Difficulty::easy
                                ? 0.00015f
                                : 0.000118f);
        }
        break;

    case State::rush:
        position_ = position_ + Float(dt) * step_vector_;
        sprite_.set_position(position_);
        shadow_.set_position(position_);
        if (timer_ > milliseconds(900)) {
            step_vector_ =
                interpolate(Vec2<Float>{0, 0}, step_vector_, dt * 0.0000060f);
        }
        if (timer_ > seconds(1) + milliseconds(350)) {
            timer_ = 0;
            state_ = State::idle1;

            send_state();
        }
        break;
    }
}


void Drone::injured(Platform& pf, Game& game, Health amount)
{
    if (sprite_.get_mix().amount_ < 180) {
        pf.sleep(2);
    }

    damage_ += amount;

    sprite_.set_mix({current_zone(game).injury_glow_color_, 255});

    debit_health(pf, amount);

    if (alive()) {
        pf.speaker().play_sound("click", 1, position_);
    } else {
        const auto add_score = 10;

        game.score() += add_score;
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


void Drone::on_collision(Platform& pf, Game& game, AlliedOrbShot&)
{
    if (not is_allied()) {
        injured(pf, game, Health{1});
    }
}


void Drone::on_collision(Platform& pf, Game& game, Player& player)
{
    if (state_ == State::rush and
        game.difficulty() not_eq Settings::Difficulty::easy) {

        injured(pf, game, get_health());
    }
}


void Drone::on_death(Platform& pf, Game& game)
{
    pf.sleep(6);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::null};

    on_enemy_destroyed(pf, game, 0, position_, 7, item_drop_vec);
}


void Drone::sync(const net_event::EnemyStateSync& state, Game&)
{
    state_ = static_cast<State>(state.state_);
    position_.x = state.x_.get();
    position_.y = state.y_.get();
    timer_ = 0;
    sprite_.set_position(position_);
    shadow_.set_position(position_);
}
