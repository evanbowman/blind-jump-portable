#include "drone.hpp"
#include "game.hpp"
#include "common.hpp"


Drone::Drone(const Vec2<Float>& pos) : Entity(Entity::Health(2)), hitbox_{&position_, {11, 11}, {8, 11}}
{
    set_position(pos);

    sprite_.set_position(pos);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    sprite_.set_texture_index(TextureMap::drone);

    shadow_.set_position(pos);
    shadow_.set_origin({7, -9});
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_texture_index(TextureMap::drop_shadow);
}


void Drone::update(Platform&, Game& game, Microseconds dt)
{
    fade_color_anim_.advance(sprite_, dt);

    if (game.player().get_position().x > position_.x) {
        sprite_.set_flip({true, false});
    } else {
        sprite_.set_flip({false, false});
    }

}


void Drone::on_collision(Platform& pf, Game& game, Laser&)
{
    sprite_.set_mix({ColorConstant::aerospace_orange, 255});
    debit_health(1);

    if (not alive()) {
        game.score() += 3;

        pf.sleep(5);

        static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                                   Item::Type::null};

        on_enemy_destroyed(pf, game, position_, 7, item_drop_vec);
    }
}
