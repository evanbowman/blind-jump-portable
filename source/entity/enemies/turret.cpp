#include "turret.hpp"
#include "common.hpp"
#include "game.hpp"
#include "number/random.hpp"
#include <algorithm>


Turret::Turret(const Vec2<Float>& pos)
    : Entity(3), state_(State::sleep), hitbox_{&position_, {16, 32}, {8, 16}},
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
    switch (state_) {
    case State::sleep:
        if (timer_ > 0) {
            timer_ -= dt;
        } else {
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
            state_ = State::open;
            timer_ = milliseconds(110);
        }
        break;

    case State::open:
        if (manhattan_length(player_pos, position_) >
            std::min(screen_size.x, screen_size.y) / 2 + 48 + 40) {
            state_ = State::closing;
        }
        if (timer_ > 0) {
            timer_ -= dt;
        } else {
            game.effects().spawn<OrbShot>(
                position_, sample<8>(game.player().get_position()), 0.00011f);
            timer_ = milliseconds(830);
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


void Turret::on_collision(Platform& pf, Game& game, Laser&)
{
    if (state_ not_eq State::closed) {
        sprite_.set_mix({ColorConstant::aerospace_orange, 255});
        debit_health(1);

        if (not alive()) {
            game.score() += 10;

            pf.sleep(5);

            static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                                       Item::Type::coin,
                                                       Item::Type::heart,
                                                       Item::Type::null};

            on_enemy_destroyed(pf, game, position_, 4, item_drop_vec);
        }
    }
}
