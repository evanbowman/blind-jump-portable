#include "theTwins.hpp"
#include "boss.hpp"
#include "game.hpp"


static const Entity::Health initial_health = 55;


void Twin::set_sprite(TextureIndex index)
{
    sprite_.set_texture_index(index);
    head_.set_texture_index(index + 1);
}


Twin* Twin::sibling(Game& game)
{
    for (auto& twin : game.enemies().get<Twin>()) {
        if (twin.get() not_eq this) {
            return twin.get();
        }
    }
    return nullptr;
}


void Twin::update_sprite()
{
    // Because we are using two adjacent horizontal sprites for this boss, we
    // need to transpose their positions if flipped, otherwise each constituent
    // sprite will be flipped in place
    if (head_.get_flip().x) {
        head_.set_position({position_.x - 18, position_.y});
        sprite_.set_position({position_.x + 14, position_.y});
        shadow_.set_position({position_.x + 2, position_.y});
        shadow2_.set_position({position_.x + 2, position_.y});
        hitbox_.dimension_.origin_ = {16, 8};
    } else {
        head_.set_position({position_.x + 18, position_.y});
        sprite_.set_position({position_.x - 14, position_.y});
        shadow_.set_position({position_.x - 2, position_.y});
        shadow2_.set_position({position_.x - 2, position_.y});
        hitbox_.dimension_.origin_ = {28, 8};
    }
}


Twin::Twin(const Vec2<Float>& position)
    : Enemy(initial_health, position, {{44, 16}, {0, 0}})
{
    head_.set_size(Sprite::Size::w32_h32);
    head_.set_origin({16, 16});

    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_origin({16, 16});

    shadow_.set_size(Sprite::Size::w32_h32);
    shadow_.set_origin({32, 9});
    shadow_.set_texture_index(33);
    shadow_.set_alpha(Sprite::Alpha::translucent);

    shadow2_.set_size(Sprite::Size::w32_h32);
    shadow2_.set_origin({0, 9});
    shadow2_.set_texture_index(34);
    shadow2_.set_alpha(Sprite::Alpha::translucent);

    set_sprite(6);
}


void Twin::update(Platform& pf, Game& game, Microseconds dt)
{
    auto face_left = [this] {
        head_.set_flip({0, 0});
        sprite_.set_flip({0, 0});
    };

    auto face_right = [this] {
        head_.set_flip({1, 0});
        sprite_.set_flip({1, 0});
    };

    auto& target = get_target(game);

    auto face_target = [this, &target, &face_left, &face_right] {
        if (target.get_position().x > position_.x) {
            face_right();
        } else {
            face_left();
        }
    };

    auto shoot_offset = [&]() -> Vec2<Float> {
        if (sprite_.get_flip().x) {
            return {21, -6};
        } else {
            return {-21, -6};
        }
    };

    constexpr Microseconds leap_duration = milliseconds(120);
    const Float movement_rate = 0.000038f;

    switch (state_) {
    case State::inactive:
        face_target();

        timer_ += dt;
        if (timer_ > seconds(3)) {
            if (auto s = sibling(game)) {
                if (s->id() < id()) {
                    pf.speaker().play_music("omega", 0);
                }
            }
            state_ = State::idle;
            timer_ = 0;
        }
        break;

    case State::long_idle:
        timer_ += dt;
        if (timer_ > seconds(2)) {
            timer_ = 0;
            leaps_ = 0;
            state_ = State::idle;
        }
        break;

    case State::idle:
        timer_ += dt;
        if (timer_ > milliseconds(400)) {
            auto sibling_state = [&] {
                if (auto s = sibling(game)) {
                    return s->state_;
                }
                return State::inactive;
            }();

            if (auto s = sibling(game)) {
                show_boss_health(pf,
                                 game,
                                 s->id() < id(),
                                 Float(get_health()) / initial_health);
            } else {
                show_boss_health(
                    pf, game, 0, Float(get_health()) / initial_health);
            }

            if (sibling_state not_eq State::open_mouth and
                sibling_state not_eq State::ranged_attack_charge and
                sibling_state not_eq State::ranged_attack and
                sibling_state not_eq State::ranged_attack_done and
                rng::choice<3>(rng::critical_state) == 0) {
                state_ = State::open_mouth;
                leaps_ = 0;
                face_target();
            } else {
                state_ = State::prep_leap;
                set_sprite(42);
            }
            timer_ = 0;
        }
        break;

    case State::open_mouth:
        timer_ += dt;
        face_target();
        if (timer_ > milliseconds(100)) {
            timer_ = 0;
            auto index = sprite_.get_texture_index();
            if (index < 14) {
                set_sprite(index + 2);
            } else {
                state_ = State::ranged_attack_charge;

                // if (rng::choice<2>(rng::critical_state)) {
                //     timer_ = seconds(1);
                // }
            }
        }
        break;

    case State::ranged_attack_charge:
        if (timer_ < seconds(1) and timer_ + dt > seconds(1)) {
            sprite_.set_mix({current_zone(game).energy_glow_color_, 0});
        }
        timer_ += dt;
        alt_timer_ += dt;
        if (alt_timer_ > milliseconds(80)) {
            alt_timer_ = 0;
            if (sprite_.get_texture_index() == 14) {
                sprite_.set_texture_index(41);
                alt_timer_ = -milliseconds(30);
            } else {
                sprite_.set_texture_index(14);
            }
        }
        if (timer_ > seconds(2) - milliseconds(100)) {
            timer_ = 0;
            state_ = State::ranged_attack;
            sprite_.set_texture_index(14);
            medium_explosion(pf, game, sprite_.get_position() + shoot_offset());
            game.camera().shake();
            target_ = target.get_position();
        }
        break;

    case State::ranged_attack:
        timer_ += dt;
        alt_timer_ += dt;

        target_ = interpolate(target.get_position(), target_, dt * 0.000016f);

        if (alt_timer_ > milliseconds(180)) {
            sprite_.set_texture_index(41);
        } else {
            sprite_.set_texture_index(14);
        }

        if (alt_timer_ > milliseconds(250)) {
            alt_timer_ = 0;
            game.effects().spawn<WandererSmallLaser>(
                position_ + shoot_offset(), target_, 0.00013f);
        }


        if (timer_ > seconds(3)) {
            state_ = State::ranged_attack_done;
        }
        break;

    case State::ranged_attack_done:
        state_ = State::idle;
        set_sprite(6);
        timer_ = milliseconds(rng::choice<800>(rng::critical_state));
        break;

    case State::prep_leap: {
        Vec2<Float> dest;
        Vec2<Float> unit;

        bool target_player = true;

        do {

            s16 dir =
                ((static_cast<float>(rng::choice<359>(rng::critical_state))) /
                 360) *
                INT16_MAX;

            unit = {(float(cosine(dir)) / INT16_MAX),
                    (float(sine(dir)) / INT16_MAX)};
            speed_ = 5.f * unit;

            if (target_player) {
                target_player = false;
                unit = direction(position_, target.get_position());
                speed_ = 5.f * unit;
            }

            const auto tolerance = milliseconds(260);
            dest = position_ +
                   speed_ * ((leap_duration + tolerance) * movement_rate);

        } while (wall_in_path(unit, position_, game, dest));
        state_ = State::crouch;
        leaps_ += 1;
        break;
    }

    case State::crouch:
        timer_ += dt;
        if (sprite_.get_texture_index() > 44) {
            position_.x += speed_.x * dt * (0.8f * movement_rate);
            position_.y += speed_.y * dt * (0.8f * movement_rate);
        }
        if (timer_ > milliseconds(80)) {
            timer_ = 0;
            auto index = sprite_.get_texture_index();
            if (index < 50) {
                set_sprite(index + 2);
            } else {
                state_ = State::leaping;
            }
        }
        break;

    case State::leaping:
        timer_ += dt;
        position_.x += speed_.x * dt * movement_rate;
        position_.y += speed_.y * dt * movement_rate;

        if (timer_ > (leap_duration * 3) / 4) {
            set_sprite(52);
        }
        if (timer_ > leap_duration) {
            state_ = State::landing;
            timer_ = 0;
        }
        break;

    case State::landing: {
        timer_ += dt;
        alt_timer_ += dt;
        if (alt_timer_ > milliseconds(100)) {
            alt_timer_ = 0;
            auto index = sprite_.get_texture_index();
            if (index not_eq 6) {
                if (index < 56) {
                    set_sprite(index + 2);
                } else {
                    set_sprite(6);
                }
            }
        }

        const auto duration = milliseconds(280);
        if (timer_ > duration) {
            if (leaps_ > 2) {
                state_ = State::long_idle;
            } else {
                state_ = State::idle;
            }
            alt_timer_ = 0;
            timer_ = 0;
            speed_ = {};
        } else {
            speed_ = interpolate(Vec2<Float>{0}, speed_, dt * 0.000016f);

            position_.x += speed_.x * dt * movement_rate;
            position_.y += speed_.y * dt * movement_rate;
        }
        break;
    }
    }

    if (speed_.x < 0) {
        face_left();
    } else if (speed_.x > 0) {
        face_right();
    }


    update_sprite();

    if (sprite_.get_mix().color_ == current_zone(game).energy_glow_color_) {
        fade_color_anim_.reverse(sprite_, dt);
    } else {
        fade_color_anim_.advance(sprite_, dt);
    }
    head_.set_mix(sprite_.get_mix());
}


void Twin::injured(Platform& pfrm, Game& game, Health amount)
{
    if (alive()) {
        pfrm.speaker().play_sound("click", 1, position_);
    }

    debit_health(pfrm, amount);

    if (auto s = sibling(game)) {
        show_boss_health(
            pfrm, game, s->id() < id(), Float(get_health()) / initial_health);
    } else {
        show_boss_health(pfrm, game, 0, Float(get_health()) / initial_health);
    }


    const auto c = current_zone(game).injury_glow_color_;

    sprite_.set_mix({c, 255});
    head_.set_mix({c, 255});
}


void Twin::on_collision(Platform& pfrm, Game& game, LaserExplosion&)
{
    injured(pfrm, game, Health{8});
}


void Twin::on_collision(Platform& pfrm, Game& game, AlliedOrbShot&)
{
    injured(pfrm, game, Health{1});
}


void Twin::on_collision(Platform& pfrm, Game& game, Player&)
{
}


void Twin::on_collision(Platform& pfrm, Game& game, Laser&)
{
    injured(pfrm, game, Health{1});
}

void Twin::on_death(Platform& pf, Game& game)
{
    big_explosion(pf, game, position_);

    hide_boss_health(game);
    pf.speaker().stop_music();

    if (sibling(game) == nullptr) {
        sprite_.set_alpha(Sprite::Alpha::transparent);
        head_.set_alpha(Sprite::Alpha::transparent);
        pf.load_sprite_texture("spritesheet_boss2_done");
        return;
    }

    push_notification(pf, game.state(), "Twin defeated...");

    game.on_timeout(pf, milliseconds(180), [this](Platform& pf, Game& game) {
        big_explosion(pf, game, {position_.x + 30, position_.y - 30});
    });

    game.on_timeout(pf, milliseconds(180), [this](Platform& pf, Game& game) {
        big_explosion(pf, game, {position_.x - 30, position_.y + 30});
    });

    game.on_timeout(pf, milliseconds(90), [this](Platform& pf, Game& game) {
        big_explosion(pf, game, {position_.x - 30, position_.y - 30});
    });

    game.on_timeout(pf, milliseconds(90), [this](Platform& pf, Game& game) {
        big_explosion(pf, game, {position_.x + 30, position_.y + 30});
    });

    auto make_scraps =
        [&] {
            auto pos = rng::sample<32>(position_, rng::utility_state);
            const auto tile_coord = to_tile_coord(pos.cast<s32>());
            const auto tile = game.tiles().get_tile(tile_coord.x, tile_coord.y);
            if (is_walkable(tile)) {
                game.details().spawn<Rubble>(pos);
            }
        };

    for (int i = 0; i < 5; ++i) {
        make_scraps();
    }

    game.camera().shake(16);
}
