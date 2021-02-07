#include "sinkhole.hpp"
#include "blind_jump/game.hpp"
#include "common.hpp"


Sinkhole::Sinkhole(const Vec2<Float>& pos)
    : Enemy(Entity::Health(999), pos, {{16, 16}, {8, 8}}), state_(State::pause),
      timer_(0), anim_timer_(0)
{
    set_position(pos);
    sprite_.set_texture_index(83);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    sprite_.set_position(pos);

    shadow_.set_position(pos);
    shadow_.set_origin({7, -9});
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_texture_index(TextureMap::drop_shadow);
}


void Sinkhole::update(Platform& pfrm, Game& game, Microseconds dt)
{
    Enemy::update(pfrm, game, dt);

    timer_ += dt;

    switch (state_) {
    case State::pause:
        if (timer_ > seconds(1)) {
            state_ = State::rise;
            timer_ = 0;
        }
        break;

    case State::rise:
        if (timer_ > milliseconds(100)) {
            const auto origin = sprite_.get_origin();
            if (origin.y < 36) {
                sprite_.set_origin({origin.x, s16(origin.y + 1)});
            } else {
                state_ = State::open;
                sprite_.set_size(Sprite::Size::w32_h32);
                sprite_.set_origin({16, origin.y});
                sprite_.set_texture_index(42);
                if (visible()) {
                    medium_explosion(pfrm, game, position_);
                }
                game.camera().shake();
            }
            timer_ = 0;
        }
        break;

    case State::open:
        if (timer_ > seconds(5)) {
            this->kill();
        }
        anim_timer_ += dt;
        if (anim_timer_ > milliseconds(70)) {
            anim_timer_ = 0;
            if (sprite_.get_texture_index() == 42) {
                sprite_.set_texture_index(43);
            } else {
                sprite_.set_texture_index(42);
            }
        }
        apply_gravity_well(position_, game.player(), 148, 128, 1.5f, 168);
        break;
    }
}


void Sinkhole::on_death(Platform& pf, Game& game)
{
    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::null};

    on_enemy_destroyed(
        pf, game, 0, {position_.x, position_.y - 32}, 0, item_drop_vec, false);
}
