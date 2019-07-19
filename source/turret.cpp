#include "turret.hpp"
#include "game.hpp"
#include <algorithm>


Turret::Turret(const Vec2<Float>& pos)
    : CollidableTemplate(HitBox{&position_, {16, 32}, {8, 16}}),
      state_(State::closed)
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
    const auto& player_pos = game.get_player().get_position();
    const auto& screen_size = pfrm.screen().size();
    switch (state_) {
    case State::closed:
        if (manhattan_length(player_pos, position_) <
            std::min(screen_size.x, screen_size.y) / 2) {
            state_ = State::opening;
        }
        break;

    case State::opening:
        if (animation_.advance(sprite_, dt)) {
            animate_shadow(shadow_, sprite_);
        }
        if (animation_.done(sprite_)) {
            state_ = State::open;
        }
        break;

    case State::open:
        if (manhattan_length(player_pos, position_) >
            std::min(screen_size.x, screen_size.y) / 2 + 48) {
            state_ = State::closing;
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
}
