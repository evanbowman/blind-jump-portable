#include "compactor.hpp"
#include "blind_jump/entity/effects/explosion.hpp"
#include "blind_jump/game.hpp"
#include "common.hpp"


static const int fall_height = 120;
static const Float start_speed = 0.00005f;


Compactor::Compactor(const Vec2<Float>& position)
    : Enemy(Entity::Health(3), position, {{16, 26}, {8, 16}}), timer_(0)
{
    sprite_.set_texture_index(106);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    sprite_.set_position({position.x, position.y});
    sprite_.set_alpha(Sprite::Alpha::transparent);

    // The compactor enemy displays a drop shadow prior to entering the level
    // map. The shadow will transition to something shaped more like the
    // character's sprite after crashing down onto the map.
    shadow_.set_position(position);
    shadow_.set_origin({8, -9});
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_texture_index(TextureMap::drop_shadow);
}


void Compactor::set_movement_vector(const Vec2<Float>& target)
{
    if (abs(target.x - position_.x) < 16) {
        if (target.y < position_.y) {
            step_vector_ = Vec2<Float>{0, -1} * start_speed;
        } else {
            step_vector_ = Vec2<Float>{0, 1} * start_speed;
        }
    } else {
        if (target.x < position_.x) {
            step_vector_ = Vec2<Float>{-1, 0} * start_speed;
        } else {
            step_vector_ = Vec2<Float>{1, 0} * start_speed;
        }
    }
}


void Compactor::update(Platform& pfrm, Game& game, Microseconds dt)
{
    switch (state_) {
    case State::sleep:
        timer_ += dt;
        if (timer_ > seconds(3)) {
            timer_ = 0;
            state_ = State::await;
        }
        break;

        // We enter this state when a network event from a peer console informs
        // us that the other game woke up the compactor enemy.
    case State::peer_triggered_fall:
        if (game.peer()) {
            set_movement_vector(game.peer()->get_position());

            state_ = State::fall;
            sprite_.set_alpha(Sprite::Alpha::opaque);
            sprite_.set_position({position_.x, position_.y - fall_height});
        } else {
            // We should never end up in this state, but in any event, I believe
            // that this would be the most sensible thing to do:
            state_ = State::await;
        }
        break;

    case State::await: {
        auto player_pos = game.player().get_position();
        if (visible() and manhattan_length(player_pos, position_) < 105) {
            if (abs(player_pos.x - position_.x) < 12 or
                abs(player_pos.y - position_.y) < 15) {

                set_movement_vector(game.player().get_position());

                state_ = State::fall;
                sprite_.set_alpha(Sprite::Alpha::opaque);
                sprite_.set_position({position_.x, position_.y - fall_height});

                net_event::EnemyStateSync s;
                // We do not initialize all of the fields in the state sync
                // event. The peer console just needs to know that someone
                // else woke up the compactor enemy.
                s.id_.set(id());

                net_event::transmit(pfrm, s);
            }
        }
        break;
    }

    case State::fall: {
        timer_ += dt;
        const auto spr_y_pos = sprite_.get_position().y;
        static const auto duration = milliseconds(190);
        if (spr_y_pos < position_.y - 1) {
            const auto yoff = ease_out(timer_, 0, fall_height, duration);

            sprite_.set_position(
                {position_.x, position_.y - (fall_height - yoff)});
        } else {
            sprite_.set_position(position_);
            state_ = State::landing;

            pfrm.speaker().play_sound("thud", 5);

            pfrm.sleep(4);
            medium_explosion(pfrm, game, position_);
            sprite_.set_mix({current_zone(game).energy_glow_color_, 255});
            sprite_.set_texture_index(107);
            game.camera().shake(16);
            game.rumble(pfrm, milliseconds(200));
            game.effects().spawn<Particle>(
                position_, current_zone(game).energy_glow_color_);
            game.effects().spawn<Particle>(
                position_, current_zone(game).energy_glow_color_);
            game.effects().spawn<Particle>(
                position_, current_zone(game).energy_glow_color_);
            timer_ = 0;

            shadow_.set_position({position_.x, position_.y + 9});
            shadow_.set_flip({false, true});
            shadow_.set_origin({8, 16});
            shadow_.set_size(Sprite::Size::w16_h32);
            shadow_.set_alpha(Sprite::Alpha::translucent);
            shadow_.set_texture_index(TextureMap::turret_shadow);
        }
        break;
    }

    case State::landing:
        state_ = State::pause;
        break;

    case State::pause:
        timer_ += dt;
        if (timer_ > [&]() {
                switch (game.persistent_data().settings_.difficulty_) {
                case Settings::Difficulty::normal:
                    break;

                case Settings::Difficulty::easy:
                    return milliseconds(400);

                case Settings::Difficulty::count:
                case Settings::Difficulty::hard:
                case Settings::Difficulty::survival:
                    return milliseconds(150);
                }
                return milliseconds(210);
            }()) {
            timer_ = 0;
            state_ = State::rush;
        }
        break;

    case State::rush:
        timer_ += dt;
        const Float target_speed = [&] {
            switch (game.persistent_data().settings_.difficulty_) {
            case Settings::Difficulty::normal:
                break;

            case Settings::Difficulty::easy:
                return 0.00018f;

            case Settings::Difficulty::count:
            case Settings::Difficulty::hard:
            case Settings::Difficulty::survival:
                return 0.00031f;
            }
            return 0.00023f;
        }();
        const auto friction_start_time = milliseconds(600);
        if (timer_ < friction_start_time) {
            if (step_vector_.x and abs(step_vector_.x) < target_speed) {
                const bool negative = step_vector_.x < 0.f;
                if (negative) {
                    step_vector_ = interpolate(Vec2<Float>{-target_speed, 0},
                                               step_vector_,
                                               dt * 0.000009f);
                } else {
                    step_vector_ = interpolate(Vec2<Float>{target_speed, 0},
                                               step_vector_,
                                               dt * 0.000009f);
                }
            } else if (step_vector_.y and abs(step_vector_.y) < target_speed) {
                const bool negative = step_vector_.y < 0.f;
                if (negative) {
                    step_vector_ = interpolate(Vec2<Float>{0, -target_speed},
                                               step_vector_,
                                               dt * 0.000009f);
                } else {
                    step_vector_ = interpolate(Vec2<Float>{0, target_speed},
                                               step_vector_,
                                               dt * 0.000009f);
                }
            }
        }

        auto coord = to_tile_coord(position_.cast<s32>());

        if (not is_walkable(game.tiles().get_tile(coord.x, coord.y))) {
            shadow_.set_alpha(Sprite::Alpha::transparent);
        } else {
            shadow_.set_alpha(Sprite::Alpha::translucent);
        }
        position_ = position_ + Float(dt) * step_vector_;
        sprite_.set_position(position_);
        shadow_.set_position({position_.x, position_.y + 9});
        if (timer_ > seconds(1) + milliseconds(200)) {
            this->kill();
        }
        if (timer_ >= friction_start_time) {
            step_vector_ =
                interpolate(Vec2<Float>{}, step_vector_, dt * 0.000012f);
        }
        break;
    }

    fade_color_anim_.advance(sprite_, dt);
}


void Compactor::injured(Platform& pf, Game& game, Health amount)
{
    pf.sleep(2);

    sprite_.set_mix({current_zone(game).injury_glow_color_, 255});

    debit_health(pf, amount);

    if (alive()) {
        pf.speaker().play_sound("click", 1, position_);
    } else {
        const auto add_score = 30;

        game.score() += add_score;
    }
}


void Compactor::on_death(Platform& pf, Game& game)
{
    pf.sleep(5);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::null};

    big_explosion(pf, game, position_);

    on_enemy_destroyed(pf, game, 0, position_, 2, item_drop_vec);
}


void Compactor::on_collision(Platform& pfrm, Game& game, LaserExplosion&)
{
}


void Compactor::on_collision(Platform& pfrm, Game& game, AlliedOrbShot&)
{
}


void Compactor::on_collision(Platform& pfrm, Game& game, PeerLaser&)
{
}


void Compactor::on_collision(Platform& pfrm, Game& game, Player&)
{
    debit_health(pfrm, 100);
}


void Compactor::on_collision(Platform& pfrm, Game& game, Laser&)
{
    // const auto player_pos = game.player().get_position();
    if (static_cast<int>(state_) > static_cast<int>(State::fall)) {
        injured(pfrm, game, 1);
    } //  else if (state_ == State::await and
    //            visible() and manhattan_length(player_pos, position_) < 150) {
    //     set_movement_vector(game.player().get_position());
    //     state_ = State::fall;
    //     sprite_.set_alpha(Sprite::Alpha::opaque);
    //     sprite_.set_position({position_.x, position_.y - fall_height});
    // }
}


void Compactor::sync(const net_event::EnemyStateSync& state, Game&)
{
    // We do not actually care what's in the sync state message, we're just
    // using the event to let a connected multiplayer console know that the
    // Compactor woke up in the other game.
    if (state_ == State::await) {
        state_ = State::peer_triggered_fall;
    }
}


Compactor::State Compactor::state() const
{
    return state_;
}
