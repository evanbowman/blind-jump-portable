#include "gatekeeper.hpp"
#include "game.hpp"


static const Entity::Health initial_health = 100;


Gatekeeper::Gatekeeper(const Vec2<Float>& position) :
    Enemy(initial_health, position, {{32, 32}, {16, 16}})
{
    sprite_.set_texture_index(7);
    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_origin({16, 16});

    head_.set_texture_index(6);
    head_.set_size(Sprite::Size::w32_h32);
    head_.set_origin({16, 48});

    shadow_.set_texture_index(41);
    shadow_.set_size(Sprite::Size::w32_h32);
    shadow_.set_origin({16, 9});
    shadow_.set_alpha(Sprite::Alpha::translucent);

    sprite_.set_position(position);
    head_.set_position(position);
    shadow_.set_position(position);
}


void Gatekeeper::update(Platform& pfrm, Game& game, Microseconds dt)
{
    fade_color_anim_.advance(sprite_, dt);
    head_.set_mix(sprite_.get_mix());
}


void Gatekeeper::on_collision(Platform& pfrm, Game& game, Laser&)
{
    debit_health(1);

    const auto c = current_zone(game).injury_glow_color_;

    sprite_.set_mix({c, 255});
    head_.set_mix({c, 255});

    show_boss_health(pfrm, game, Float(get_health()) / initial_health);
}
