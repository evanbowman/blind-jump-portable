#include "turret.hpp"
#include "blind_jump/game.hpp"
#include "common.hpp"
#include "number/random.hpp"
#include <algorithm>


static Microseconds reload(Level level)
{
    static constexpr const Microseconds reload_time{milliseconds(980)};

    if (level < boss_0_level) {
        return reload_time;
    } else if (level < boss_1_level) {
        return reload_time * 0.9f;
    } else {
        return reload_time * 0.9f;
    }
}


static Microseconds pause_after_open(Game& game, Level level)
{
    if (level < boss_0_level) {
        switch (game.difficulty()) {
        case Settings::Difficulty::easy:
            return milliseconds(375);

        case Settings::Difficulty::count:
        case Settings::Difficulty::normal:
            return milliseconds(274);

        case Settings::Difficulty::survival:
        case Settings::Difficulty::hard:
            return milliseconds(210);
        }
    } else if (level < boss_1_level) {
        switch (game.difficulty()) {
        case Settings::Difficulty::easy:
            return milliseconds(341);

        case Settings::Difficulty::count:
        case Settings::Difficulty::normal:
            return milliseconds(240);

        case Settings::Difficulty::survival:
        case Settings::Difficulty::hard:
            return milliseconds(180);
        }
    } else {
        switch (game.difficulty()) {
        case Settings::Difficulty::easy:
            return milliseconds(311);

        case Settings::Difficulty::count:
        case Settings::Difficulty::normal:
            return milliseconds(230);

        case Settings::Difficulty::survival:
        case Settings::Difficulty::hard:
            return milliseconds(170);
        }
    }
    return milliseconds(200);
}


Turret::Turret(const Vec2<Float>& pos)
    : Enemy(Entity::Health(3), pos, {{16, 32}, {8, 16}}), state_(State::sleep),
      timer_(seconds(2))
{
    set_position(pos);
    sprite_.set_position(pos);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});

    shadow_.set_position({pos.x, pos.y + 14});
    shadow_.set_origin({8, 16});
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_texture_index(TextureMap::turret_shadow);

    animation_.bind(sprite_);
}


static void animate_shadow(Sprite& shadow, Sprite& turret_spr)
{
    const auto& pos = turret_spr.get_position();
    shadow.set_position(
        {pos.x,
         pos.y + 14 +
             (turret_spr.get_texture_index() - TextureMap::turret) / 2});
}


void Turret::update(Platform& pfrm, Game& game, Microseconds dt)
{
    Enemy::update(pfrm, game, dt);

    const auto& target = get_target(game);
    const auto& target_pos = target.get_position();
    const auto& screen_size = pfrm.screen().size();

    static const auto bullet_speed = 0.00011f;

    auto aim = [&] { return rng::sample<8>(target_pos, rng::critical_state); };

    auto origin = [&] { return position_ + Vec2<Float>{0.f, 4.f}; };

    auto try_close = [&] {
        if (manhattan_length(target_pos, position_) >
            std::min(screen_size.x, screen_size.y) / 2 + 48 + 40) {
            state_ = State::closing;
        }
    };

    switch (state_) {
    case State::sleep:
        if (timer_ > 0) {
            timer_ -= dt;
        } else {
            if (game.level() > boss_1_level) {
                add_health(1);
            } else if (game.level() > boss_0_level) {
                add_health(1);
            }
            if (game.difficulty() == Settings::Difficulty::hard) {
                add_health(1);
            } else if (game.difficulty() == Settings::Difficulty::easy) {
                set_health(get_health() - 1);
            }
            state_ = State::closed;
        }
        break;

    case State::closed:
        if (visible() or pfrm.network_peer().is_connected()) {
            if (manhattan_length(target_pos, position_) <
                std::min(screen_size.x, screen_size.y) / 2 + 15) {
                state_ = State::opening;
            }
        }
        break;

    case State::opening:
        if (animation_.advance(sprite_, dt)) {
            animate_shadow(shadow_, sprite_);
        }
        if (animation_.done(sprite_)) {
            state_ = State::open1;
            timer_ = pause_after_open(game, game.level());
        }
        break;

    case State::open1:
        try_close();


        if (timer_ > 0) {
            timer_ -= dt;
        } else {
            pfrm.speaker().play_sound("laser1", 4, position_);

            this->shoot(pfrm, game, origin(), aim(), bullet_speed);
            timer_ = reload(game.level());

            if (game.difficulty() == Settings::Difficulty::easy) {
                timer_ *= 1.25;
            }

            if (game.level() > boss_1_level) {
                state_ = State::open2;
            }
        }

        break;

    case State::open2:
        try_close();

        if (timer_ > 0) {
            timer_ -= dt;
        } else {
            pfrm.speaker().play_sound("laser1", 4, position_);

            const auto angle = 25;

            if (auto shot =
                    this->shoot(pfrm, game, origin(), aim(), bullet_speed)) {
                shot->rotate(angle);
            }

            if (auto shot =
                    this->shoot(pfrm, game, origin(), aim(), bullet_speed)) {
                shot->rotate(365 - angle);
            }

            timer_ = reload(game.level());

            state_ = State::open1;
        }
        break;

    case State::closing:
        if (animation_.reverse(sprite_, dt)) {
            animate_shadow(shadow_, sprite_);
        }
        if (animation_.at_beginning(sprite_)) {
            state_ = State::closed;
        }
        break;
    }

    fade_color_anim_.advance(sprite_, dt);
}


void Turret::injured(Platform& pf, Game& game, Health amount)
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
        const auto add_score = 12;

        game.score() += add_score;
    }
}

void Turret::on_collision(Platform& pf, Game& game, Laser&)
{
    if (state_ not_eq State::closed) {
        injured(pf, game, Health{1});
    }
}


void Turret::on_collision(Platform& pf, Game& game, LaserExplosion&)
{
    injured(pf, game, Health{8});
}


void Turret::on_collision(Platform& pf, Game& game, AlliedOrbShot&)
{
    if (not is_allied()) {
        injured(pf, game, Health{1});
    }
}


void Turret::on_death(Platform& pf, Game& game)
{
    pf.sleep(6);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::coin,
                                               Item::Type::heart,
                                               Item::Type::null};

    on_enemy_destroyed(pf, game, 0, position_, 4, item_drop_vec);
}


void Turret::sync(const net_event::EnemyStateSync& state, Game& game)
{
    state_ = static_cast<State>(state.state_);
    timer_ = reload(game.level()) - milliseconds(80);
}
