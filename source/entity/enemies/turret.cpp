#include "turret.hpp"
#include "common.hpp"
#include "game.hpp"
#include "number/random.hpp"
#include <algorithm>


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
    const auto& player_pos = game.player().get_position();
    const auto& screen_size = pfrm.screen().size();

    static const auto bullet_speed = 0.00011f;

    auto target = [&] { return sample<8>(game.player().get_position()); };

    auto origin = [&] { return position_ + Vec2<Float>{0.f, 4.f}; };

    auto try_close = [&] {
        if (manhattan_length(player_pos, position_) >
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
                add_health(2);
            } else if (game.level() > boss_0_level) {
                add_health(1);
            }
            state_ = State::closed;
        }
        break;

    case State::closed:
        if (visible()) {
            if (manhattan_length(player_pos, position_) <
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
            timer_ = milliseconds(110);
        }
        break;

    case State::open1:
        try_close();

        static constexpr const Microseconds reload{milliseconds(830)};

        if (timer_ > 0) {
            timer_ -= dt;
        } else {
            pfrm.speaker().play_sound("laser1", 4);

            game.effects().spawn<OrbShot>(origin(), target(), bullet_speed);
            timer_ = reload;

            if (game.level() > boss_1_level) {
                state_ = State::open2;
                timer_ = reload * 0.75;
            }
        }

        break;

    case State::open2:
        try_close();

        if (timer_ > 0) {
            timer_ -= dt;
        } else {
            pfrm.speaker().play_sound("laser1", 4);

            const auto angle = 25;

            if (game.effects().spawn<OrbShot>(
                    origin(), target(), bullet_speed)) {
                (*game.effects().get<OrbShot>().begin())->rotate(angle);
            }

            if (game.effects().spawn<OrbShot>(
                    origin(), target(), bullet_speed)) {
                (*game.effects().get<OrbShot>().begin())->rotate(365 - angle);
            }

            timer_ = reload * 0.75;

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
    sprite_.set_mix({current_zone(game).injury_glow_color_, 255});
    debit_health(amount);

    if (alive()) {
        pf.speaker().play_sound("click", 1);
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


void Turret::on_death(Platform& pf, Game& game)
{
    game.score() += 12;

    pf.sleep(5);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::coin,
                                               Item::Type::heart,
                                               Item::Type::null};

    on_enemy_destroyed(pf, game, position_, 4, item_drop_vec);
}
