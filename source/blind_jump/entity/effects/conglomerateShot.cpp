#include "conglomerateShot.hpp"
#include "blind_jump/game.hpp"
#include "number/random.hpp"


ConglomerateShot::ConglomerateShot(const Vec2<Float>& position,
                                   const Vec2<Float>& target,
                                   Float speed,
                                   Microseconds duration)
    : Projectile(position, target, speed), timer_(duration),
      flicker_timer_(0), hitbox_{&position_, {{14, 14}, {16, 16}}}
{
    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_texture_index(58);
    sprite_.set_origin({16, 16});
}


void ConglomerateShot::update(Platform& pf, Game& game, Microseconds dt)
{
    Projectile::update(pf, game, dt);

    timer_ -= dt;
    flicker_timer_ += dt;

    // FIXME: For the boss level, we spawn alternate-colored conglomerate shots,
    // due to the differing base color of the effect in the spritesheets.
    if (game.level() == boss_2_level) {
        if (flicker_timer_ > milliseconds(20)) {
            flicker_timer_ = 0;

            switch (rng::choice<3>(rng::utility_state)) {
            case 0:
                sprite_.set_mix({});
                break;

            case 1:
                sprite_.set_mix({ColorConstant::picton_blue, 128});
                break;

            case 2:
                sprite_.set_mix({ColorConstant::picton_blue, 255});
                break;
            }
        }
    }

    if (timer_ <= 0) {
        Entity::kill();
    }
}


void ConglomerateShot::on_collision(Platform& pf, Game&, Player&)
{
    Entity::kill();
}


void ConglomerateShot::on_death(Platform& pf, Game& game)
{
    const auto speed = [&] {
        switch (game.persistent_data().settings_.difficulty_) {
        case Settings::Difficulty::easy:
            return 0.00008f;

        case Settings::Difficulty::survival:
        case Settings::Difficulty::hard:
            return 0.00016f;

        default:
            break;
        }

        return 0.00013f;
    }();

    if (game.level() == boss_2_level) {
        game.effects().spawn<WandererSmallLaser>(
            position_, Vec2<Float>{position_.x + 50, position_.y}, speed);
        game.effects().spawn<WandererSmallLaser>(
            position_, Vec2<Float>{position_.x - 50, position_.y}, speed);
        game.effects().spawn<WandererSmallLaser>(
            position_, Vec2<Float>{position_.x, position_.y + 50}, speed);
        game.effects().spawn<WandererSmallLaser>(
            position_, Vec2<Float>{position_.x, position_.y - 50}, speed);
    } else {
        game.effects().spawn<OrbShot>(
            position_, Vec2<Float>{position_.x + 50, position_.y}, speed);
        game.effects().spawn<OrbShot>(
            position_, Vec2<Float>{position_.x - 50, position_.y}, speed);
        game.effects().spawn<OrbShot>(
            position_, Vec2<Float>{position_.x, position_.y + 50}, speed);
        game.effects().spawn<OrbShot>(
            position_, Vec2<Float>{position_.x, position_.y - 50}, speed);
    }


    game.camera().shake();

    medium_explosion(pf, game, position_);
    pf.speaker().play_sound("explosion1", 3, position_);
}
