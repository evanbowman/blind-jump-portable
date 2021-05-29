#include "scarecrow.hpp"
#include "blind_jump/game.hpp"
#include "common.hpp"
#include "graphics/overlay.hpp"
#include "number/random.hpp"


static const Float long_jump_speed(0.00015f);


Scarecrow::Scarecrow(const Vec2<Float>& position)
    : Enemy(Entity::Health(3), position, {{16, 32}, {8, 16}}), timer_(0),
      bounce_timer_(0), shadow_check_timer_(0), hit_(false)
{
    position_ = {position.x, position.y - 32};

    sprite_.set_texture_index(TextureMap::scarecrow_top);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    sprite_.set_position(position_);

    leg_.set_texture_index(TextureMap::scarecrow_bottom);
    leg_.set_size(Sprite::Size::w16_h32);
    leg_.set_origin({8, -16});
    leg_.set_position(position_);

    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -32});
    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_position(position_);

    anchor_ = to_tile_coord(position.cast<s32>());
}


void Scarecrow::update(Platform& pfrm, Game& game, Microseconds dt)
{
    Enemy::update(pfrm, game, dt);

    auto face_left = [this] {
        sprite_.set_flip({0, 0});
        leg_.set_flip({0, 0});
    };

    auto face_right = [this] {
        sprite_.set_flip({1, 0});
        leg_.set_flip({1, 0});
    };

    auto& target = get_target(game);

    auto face_target = [this, &target, &face_left, &face_right] {
        if (target.get_position().x > position_.x) {
            face_right();
        } else {
            face_left();
        }
    };

    face_target();

    fade_color_anim_.advance(sprite_, dt);
    leg_.set_mix(sprite_.get_mix());

    switch (state_) {
    case State::sleep:
        timer_ += dt;
        if (timer_ > seconds(2) or visible()) {
            timer_ = 0;
            state_ = State::inactive;

            if (game.difficulty() == Settings::Difficulty::hard) {
                add_health(1);
            } else if (game.difficulty() == Settings::Difficulty::easy) {
                set_health(get_health() - 1);
            }
        }
        break;

    case State::inactive:
        if (manhattan_length(target.get_position(), position_) < 280) {
            state_ = State::idle_wait;
        }
        break;

    case State::idle_crouch: {
        const auto sprite_pos = sprite_.get_position();
        timer_ += dt;
        if (timer_ > milliseconds(35)) {
            timer_ = 0;
            if (sprite_pos.y < position_.y + 3) {
                sprite_.set_position({sprite_pos.x, sprite_pos.y + 1});
            } else {
                state_ = State::idle_wait;
            }
        }
        break;
    }

    case State::idle_wait:
        timer_ += dt;
        if (timer_ > milliseconds(100)) {
            timer_ = 0;
            sprite_.set_texture_index(TextureMap::scarecrow_top_2);
            state_ = State::idle_jump;
        }
        break;

    case State::idle_jump: {
        const auto sprite_pos = sprite_.get_position();
        timer_ += dt;
        if (timer_ > milliseconds(12)) {
            timer_ = 0;
            if (sprite_pos.y > position_.y + 1) {
                sprite_.set_position({sprite_pos.x, sprite_pos.y - 1});
            } else {
                sprite_.set_position(position_);

                move_vec_ = direction(position_,
                                      rng::sample<32>(to_world_coord(anchor_),
                                                      rng::critical_state)) *
                            0.000005f;

                bounce_timer_ = 0;
                state_ = State::idle_airborne;
            }
        }

        break;
    }

    case State::idle_airborne:
        bounce_timer_ += dt;
        // if (visible())
        {

            position_ = position_ + Float(dt) * move_vec_;

            const Float offset =
                8 * float(sine(4 * 3.14f * 0.0027f * bounce_timer_ + 180)) /
                std::numeric_limits<s16>::max();


            auto pos = sprite_.get_position();
            if (pos.y < position_.y - abs(offset) or abs(offset) > 7) {
                sprite_.set_texture_index(TextureMap::scarecrow_top);

                if (int(abs(offset)) == 0) {
                    if (rng::choice<4>(rng::critical_state) and not hit_) {
                        state_ = State::idle_crouch;
                    } else {
                        state_ = State::long_crouch;
                        hit_ = false;
                    }
                    bounce_timer_ = 0;
                }
            }

            sprite_.set_position({position_.x, position_.y - abs(offset)});

            leg_.set_position({position_.x, position_.y - abs(offset)});
            shadow_.set_position(position_);
        }
        break;

    case State::long_crouch: {
        const auto sprite_pos = sprite_.get_position();
        timer_ += dt;
        if (timer_ > milliseconds(20)) {
            timer_ = 0;
            if (sprite_pos.y < position_.y + 5) {
                sprite_.set_position({sprite_pos.x, sprite_pos.y + 1});
            } else {
                state_ = State::long_wait;
            }
        }
        break;
    }

    case State::long_wait:
        timer_ += dt;
        if (timer_ > milliseconds(200)) {
            timer_ = 0;
            sprite_.set_texture_index(TextureMap::scarecrow_top_2);
            state_ = State::long_jump;
        }
        break;

    case State::long_jump: {
        const auto sprite_pos = sprite_.get_position();
        timer_ += dt;
        if (timer_ > milliseconds(7)) {
            timer_ = 0;
            if (sprite_pos.y > position_.y + 1) {
                sprite_.set_position({sprite_pos.x, sprite_pos.y - 1});
            } else {
                sprite_.set_position(position_);

                auto target_pos = target.get_position();

                // const auto dist = distance(position_, player_pos);
                // if (dist > 64) {
                //     player_pos = interpolate(player_pos, position_, 64 / dist);
                // }

                const auto target_coord = to_tile_coord(target_pos.cast<s32>());

                for (int i = 0; i < 200; ++i) {
                    auto selected = to_tile_coord(
                        rng::sample<64>(target_pos, rng::critical_state)
                            .cast<s32>());

                    // This is just because the enemy is tall, and we do not
                    // want it to jump to a coordinate, such that its head is
                    // cropped off of the screen;
                    selected.y += 1;

                    const auto t =
                        game.tiles().get_tile(selected.x, selected.y);
                    if (is_walkable(t) and
                        manhattan_length(selected, target_coord) > 3 and
                        selected not_eq anchor_) {
                        anchor_ = selected;
                        break;
                    }
                }

                auto target = to_world_coord(anchor_);
                target.y -= 32;
                target.x += 16;
                move_vec_ = direction(position_, target) * long_jump_speed;

                const auto vec = target - position_;
                const auto magnitude =
                    sqrt_approx(vec.x * vec.x + vec.y * vec.y);

                timer_ = abs(magnitude) / long_jump_speed;

                bounce_timer_ = timer_;
                state_ = State::long_airborne;
            }
        }

        break;
    }

    case State::long_airborne: {
        if (timer_ > 0) {

            auto coord = to_tile_coord(position_.cast<s32>());
            coord.y += 1;

            if (visible()) {
                shadow_check_timer_ += dt;
                if (shadow_check_timer_ > milliseconds(100)) {
                    shadow_check_timer_ = milliseconds(0);

                    // Hide the shadow when we're jumping over empty space
                    if (not is_walkable(
                            game.tiles().get_tile(coord.x, coord.y))) {
                        shadow_.set_alpha(Sprite::Alpha::transparent);
                    } else {
                        shadow_.set_alpha(Sprite::Alpha::translucent);
                    }
                }
            }

            timer_ -= dt;

            position_ = position_ + Float(dt) * move_vec_;

            const Float offset =
                8 *
                float(sine(4 * 3.14f *
                               (360 * (1 - (bounce_timer_ - timer_) /
                                               Float(bounce_timer_))) +
                           180)) /
                std::numeric_limits<s16>::max();

            sprite_.set_position({position_.x, position_.y - abs(offset)});
            leg_.set_position({position_.x, position_.y - abs(offset)});
            shadow_.set_position(position_);

        } else {
            timer_ = 0;
            bounce_timer_ = 0;
            state_ = State::landing;
            shadow_.set_alpha(Sprite::Alpha::translucent);

            sprite_.set_position(position_);
            leg_.set_position(position_);
            shadow_.set_position(position_);
        }
        break;
    }

    case State::landing: {
        const auto sprite_pos = sprite_.get_position();
        timer_ += dt;
        if (timer_ > milliseconds(35)) {
            timer_ = 0;
            if (sprite_pos.y < position_.y + 3) {
                sprite_.set_position({sprite_pos.x, sprite_pos.y + 1});
            } else {
                state_ = State::attack;
            }
        }
        break;
    }

    case State::attack: {
        timer_ += dt;
        if (timer_ > [&] {
                if (game.difficulty() == Settings::Difficulty::easy) {
                    return milliseconds(300);
                } else {
                    return milliseconds(200);
                }
            }()) {
            timer_ = 0;
            if (visible()) {
                pfrm.speaker().play_sound("laser1", 4, position_);

                const auto conglomerate_shot_level = [&] {
                    if (game.difficulty() == Settings::Difficulty::easy) {
                        return boss_2_level;
                    } else {
                        return boss_1_level;
                    }
                }();

                if (not is_allied() and
                    game.level() > conglomerate_shot_level) {
                    Float shot_speed = 0.00011f;
                    if (game.difficulty() == Settings::Difficulty::easy) {
                        shot_speed = 0.00008f;
                    }
                    game.effects().spawn<ConglomerateShot>(
                        position_,
                        rng::sample<8>(target.get_position(),
                                       rng::critical_state),
                        shot_speed);
                } else {
                    this->shoot(pfrm,
                                game,
                                position_,
                                rng::sample<8>(target.get_position(),
                                               rng::critical_state),
                                0.00010f);
                }
            }
            state_ = State::idle_wait;

            if (&target == &game.player()) {
                const auto int_pos = position_.cast<s16>();

                net_event::EnemyStateSync s;
                s.state_ = static_cast<u8>(state_);
                s.x_.set(int_pos.x);
                s.y_.set(int_pos.y);
                s.id_.set(id());

                net_event::transmit(pfrm, s);
            }
        }
        break;
    }
    }
}


void Scarecrow::sync(const net_event::EnemyStateSync& state, Game&)
{
    state_ = static_cast<State>(state.state_);
    position_.x = state.x_.get();
    position_.y = state.y_.get();
    timer_ = 0;
    sprite_.set_position(position_);
    shadow_.set_position(position_);
}


void Scarecrow::injured(Platform& pf, Game& game, Health amount)
{
    if (sprite_.get_mix().amount_ < 180) {
        pf.sleep(2);
    }

    damage_ += amount;

    debit_health(pf, amount);

    if (alive()) {
        pf.speaker().play_sound("click", 1, position_);
    } else {
        const auto add_score = 15;

        game.score() += add_score;
    }

    hit_ = true;

    const auto c = current_zone(game).injury_glow_color_;

    sprite_.set_mix({c, 255});
    leg_.set_mix({c, 255});
}


void Scarecrow::on_collision(Platform& pf, Game& game, Laser&)
{
    injured(pf, game, Health{1});
}


void Scarecrow::on_collision(Platform& pf, Game& game, LaserExplosion&)
{
    injured(pf, game, Health{8});
}


void Scarecrow::on_collision(Platform& pf, Game& game, AlliedOrbShot&)
{
    if (not is_allied()) {
        injured(pf, game, Health{1});
    }
}


void Scarecrow::on_death(Platform& pf, Game& game)
{
    pf.sleep(6);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::coin,
                                               Item::Type::coin,
                                               Item::Type::heart,
                                               Item::Type::null};

    on_enemy_destroyed(pf,
                       game,
                       -16,
                       Vec2<Float>{position_.x, position_.y + 32},
                       3,
                       item_drop_vec);
}
