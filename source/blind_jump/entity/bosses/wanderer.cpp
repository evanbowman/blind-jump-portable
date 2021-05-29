#include "wanderer.hpp"
#include "blind_jump/entity/effects/explosion.hpp"
#include "blind_jump/game.hpp"
#include "boss.hpp"
#include "number/random.hpp"
#include "wallCollision.hpp"


static const char* boss_music = "omega";


static const Entity::Health initial_health = 100;


static Entity::Health get_initial_health(Game& game)
{
    if (game.difficulty() == Settings::Difficulty::easy) {
        return 70;
    } else {
        return 100;
    }
}


Wanderer::Wanderer(const Vec2<Float>& position)
    : Enemy(initial_health, position, {{16, 38}, {8, 24}}), timer_(0),
      timer2_(0), chase_player_(0), dashes_remaining_(0), bullet_spread_gap_(0)
{
    sprite_.set_texture_index(12);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});

    head_.set_texture_index(13);
    head_.set_size(Sprite::Size::w16_h32);
    head_.set_origin({8, 32});

    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -9});
    shadow_.set_alpha(Sprite::Alpha::translucent);

    sprite_.set_position(position);
    head_.set_position({position_.x, position_.y - 16});
    shadow_.set_position(position);
}


bool Wanderer::second_form(Game& game) const
{
    return get_health() < get_initial_health(game) - 42;
}


void Wanderer::update(Platform& pf, Game& game, Microseconds dt)
{
    auto face_left = [this] {
        sprite_.set_flip({1, 0});
        head_.set_flip({1, 0});
    };

    auto face_right = [this] {
        sprite_.set_flip({0, 0});
        head_.set_flip({0, 0});
    };

    auto& target = boss_get_target(game);


    auto face_player = [this, &target, &face_left, &face_right] {
        if (target.get_position().x > position_.x) {
            face_right();
        } else {
            face_left();
        }
    };

    auto shoot_offset = [&]() -> Vec2<Float> {
        if (sprite_.get_flip().x) {
            return {-4, -6};
        } else {
            return {4, -6};
        }
    };

    auto to_wide_sprite = [&] {
        sprite_.set_size(Sprite::Size::w32_h32);
        sprite_.set_origin({16, 16});
        head_.set_size(Sprite::Size::w32_h32);
        head_.set_origin({16, 32});
    };

    auto to_narrow_sprite = [&] {
        sprite_.set_size(Sprite::Size::w16_h32);
        sprite_.set_origin({8, 16});
        head_.set_size(Sprite::Size::w16_h32);
        head_.set_origin({8, 32});
    };

    const auto dash_duration =
        second_form(game) ? milliseconds(500) : milliseconds(650);
    const float movement_rate = second_form(game) ? 0.000029f : 0.000022f;

    constexpr Angle scattershot_inflection = 90;

    // if (third_form()) {
    //     state_ = State::final_form;
    // }

    switch (state_) {
    case State::sleep:
        if (visible()) {
            // Start facing away from the player
            if (target.get_position().x > position_.x) {
                face_left();
            } else {
                face_right();
            }

            timer_ += dt;
            if (timer_ > seconds(1)) {
                state_ = State::still;
                timer_ = 0;

                set_health(get_initial_health(game));

                pf.speaker().play_music(boss_music, 0);

                show_boss_health(pf,
                                 game,
                                 0,
                                 Float(get_health()) /
                                     get_initial_health(game));
            }
        }
        break;

    case State::after_dash:
        timer_ += dt;

        if (timer_ > milliseconds(150)) {
            to_narrow_sprite();
            sprite_.set_texture_index(12);
            head_.set_texture_index(13);

            timer_ = 0;
            state_ = State::still;
        }
        break;


    case State::still: {
        if (timer_ > milliseconds(70)) {
            face_player();
        }

        timer_ += dt;
        if (timer_ > milliseconds(200)) {
            timer_ = 0;

            auto t_coord = to_tile_coord(position_.cast<s32>());
            const auto tile = game.tiles().get_tile(t_coord.x, t_coord.y);

            if ((rng::choice<2>(rng::critical_state) and not is_border(tile) and
                 not chase_player_) or
                (not is_border(tile) and dashes_remaining_ == 0)) {
                state_ = State::draw_weapon;

                to_wide_sprite();

                sprite_.set_texture_index(7);
                head_.set_texture_index(41);

            } else {
                state_ = State::prep_dash;
                if (second_form(game)) {
                    if (rng::choice<3>(rng::critical_state) == 0) {
                        chase_player_ = 3;
                    }
                }
            }
        }
        break;
    }

    case State::draw_weapon: {
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(90)) {
            timer_ = 0;

            const auto index = sprite_.get_texture_index();
            if (index < 12) {
                sprite_.set_texture_index(index + 1);
                head_.set_texture_index(
                    std::min(u16(45), u16(head_.get_texture_index() + 1)));

            } else {
                timer_ = 0;

                if (distance(position_, target.get_position()) > 80 and
                    rng::choice<2>(rng::critical_state) and
                    get_health() < get_initial_health(game) - 10) {
                    state_ = State::big_laser_shooting;
                    sprite_.set_mix({ColorConstant::electric_blue, 0});
                    head_.set_mix({ColorConstant::electric_blue, 0});

                    if (auto dt = pf.make_dynamic_texture()) {
                        game.effects().spawn<DynamicEffect>(position_ +
                                                                shoot_offset(),
                                                            *dt,
                                                            milliseconds(40),
                                                            89,
                                                            5);
                    }

                } else {
                    state_ = State::small_laser_prep;
                }
            }
        }

        break;
    }

    case State::small_laser_prep: {
        bullet_spread_gap_ =
            rng::choice<scattershot_inflection - 10>(rng::critical_state) + 5;
        scattershot_target_ = target.get_position();
        state_ = State::small_laser;
        break;
    }

    case State::small_laser:
        timer_ += dt;
        timer2_ += dt;

        if (timer_ > [&] {
                switch (game.difficulty()) {
                case Settings::Difficulty::easy:
                    if (second_form(game)) {
                        return milliseconds(100);
                    } else {
                        return milliseconds(130);
                    }
                    break;

                case Settings::Difficulty::count:
                case Settings::Difficulty::normal:
                    if (second_form(game)) {
                        return milliseconds(82);
                    } else {
                        return milliseconds(90);
                    }
                    break;

                case Settings::Difficulty::survival:
                case Settings::Difficulty::hard:
                    if (second_form(game)) {
                        return milliseconds(66);
                    } else {
                        return milliseconds(70);
                    }
                }
                return milliseconds(90);
            }()) {
            game.effects().spawn<WandererSmallLaser>(
                position_ + shoot_offset(), scattershot_target_, 0.00013f);

            Angle angle;
            const bool avoid_region = rng::choice<3>(rng::critical_state);
            do {
                angle =
                    rng::choice<scattershot_inflection>(rng::critical_state);
            } while (abs(angle - bullet_spread_gap_) < 2 and not avoid_region);

            if (angle > scattershot_inflection / 2) {
                angle = 360 - angle / 2;
            }

            (*game.effects().get<WandererSmallLaser>().begin())->rotate(angle);

            timer_ = 0;
        }

        if (timer2_ > milliseconds(1200)) {
            timer2_ = 0;
            timer_ = 0;
            state_ = State::done_shooting;
        }
        break;

    case State::big_laser_shooting:
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(10)) {
            timer_ = 0;
            state_ = State::big_laser1;
        }
        break;

    case State::big_laser1: {
        face_player();

        timer_ += dt;
        if (timer_ > [&]() {
                switch (game.difficulty()) {
                case Settings::Difficulty::easy:
                    return milliseconds(720);

                case Settings::Difficulty::count:
                case Settings::Difficulty::normal:
                    break;
                case Settings::Difficulty::survival:
                case Settings::Difficulty::hard:
                    return milliseconds(580);
                }
                return milliseconds(640);
            }()) {
            game.camera().shake();
            game.rumble(pf, milliseconds(100));
            medium_explosion(pf, game, position_ + shoot_offset());

            Float speed = 0.00028f;
            if (game.difficulty() == Settings::Difficulty::easy) {
                speed = 0.000215f;
            }

            game.effects().spawn<WandererBigLaser>(
                position_ + shoot_offset(),
                rng::sample<8>(target.get_position(), rng::critical_state),
                speed);

            timer_ = 0;
            state_ = State::big_laser2;
        }
        break;
    }

    case State::big_laser2: {
        face_player();

        Float speed = 0.00021f;
        if (game.difficulty() == Settings::Difficulty::easy) {
            speed = 0.00016f;
        }

        const auto wait_time = [&]() {
            if (game.difficulty() == Settings::Difficulty::easy) {
                return milliseconds(240);
            } else {
                return milliseconds(180);
            }
        }();

        timer_ += dt;
        if (timer_ > wait_time) {
            game.effects().spawn<WandererBigLaser>(
                position_ + shoot_offset(),
                rng::sample<12>(target.get_position(), rng::critical_state),
                speed);
            timer_ = 0;

            if (game.difficulty() == Settings::Difficulty::easy) {
                state_ = State::done_shooting;
            } else {
                state_ = State::big_laser3;
            }
        }
        break;
    }

    case State::final_form:
        break;

    case State::big_laser3:
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(180)) {
            game.effects().spawn<WandererBigLaser>(
                position_ + shoot_offset(),
                rng::sample<22>(target.get_position(), rng::critical_state),
                0.00015f);
            timer_ = 0;
            state_ = State::done_shooting;

            if (pf.network_peer().is_connected() and
                pf.network_peer().is_host()) {
                if (rng::choice<2>(rng::critical_state) == 0) {
                    const auto int_pos = position_.cast<s16>();

                    net_event::EnemyStateSync s;
                    s.state_ = static_cast<u8>(state_);
                    s.x_.set(int_pos.x);
                    s.y_.set(int_pos.y);
                    s.id_.set(id());

                    net_event::transmit(pf, s);


                    net_event::BossSwapTarget t;
                    t.target_.set(game.get_boss_target());
                    game.set_boss_target(not game.get_boss_target());

                    net_event::transmit(pf, t);
                }
            }
        }
        break;

    case State::done_shooting:
        timer_ += dt;
        if (timer_ > milliseconds(100)) {
            timer_ = 0;
            dashes_remaining_ = 3;
            state_ = State::prep_dash;

            to_narrow_sprite();

            sprite_.set_texture_index(12);
            head_.set_texture_index(13);
        }
        break;

    case State::prep_dash:
        timer_ += dt;
        if (timer_ > milliseconds(200)) {
            timer_ = 0;
            timer2_ = 0;

            if (dashes_remaining_) {
                dashes_remaining_--;
            }

            to_wide_sprite();

            sprite_.set_texture_index(13);
            head_.set_texture_index(46);

            state_ = State::dash;

            s16 dir;
            Vec2<Float> dest;
            Vec2<Float> unit;

            bool chase = rng::choice<2>(rng::critical_state);
            int tries = 0;

            do {
                if (tries) {
                    chase = false;
                    chase_player_ = 0;
                }

                dir = ((static_cast<float>(
                           rng::choice<359>(rng::critical_state))) /
                       360) *
                      INT16_MAX;

                unit = {(float(cosine(dir)) / INT16_MAX),
                        (float(sine(dir)) / INT16_MAX)};
                speed_ = 5.f * unit;

                if ((not second_form(game) and
                     rng::choice<3>(rng::critical_state) == 0) or
                    (chase_player_ and chase)) {
                    if (chase_player_) {
                        chase_player_--;
                    }
                    unit = direction(position_, target.get_position());
                    speed_ = 5.f * unit;
                }

                tries++;

                const auto tolerance = milliseconds(70);
                dest = position_ +
                       speed_ * ((dash_duration + tolerance) * movement_rate);

            } while (abs(speed_.x) < 1 // The dashing animation just looks
                                       // strange when the character is moving
                                       // vertically.
                     or wall_in_path(unit, position_, game, dest));
        }
        break;

    case State::dash: {
        timer_ += dt;
        timer2_ += dt;

        if (timer_ > milliseconds(100)) {
            timer_ = 0;

            if (sprite_.get_texture_index() == 13) {
                sprite_.set_texture_index(14);
                head_.set_texture_index(47);
            } else {
                sprite_.set_texture_index(13);
                head_.set_texture_index(46);
            }
        }

        auto transition = [&] {
            state_ = State::after_dash;

            sprite_.set_texture_index(50);
            head_.set_texture_index(51);

            timer_ = 0;
            timer2_ = 0;
        };

        if (timer2_ > dash_duration) {
            transition();
        }

        if (speed_.x < 0) {
            face_left();
        } else {
            face_right();
        }

        position_.x += speed_.x * dt * movement_rate;
        position_.y += speed_.y * dt * movement_rate;

        sprite_.set_position(position_);
        head_.set_position({position_.x, position_.y - 16});
        shadow_.set_position(position_);

        break;
    }
    }

    if (sprite_.get_mix().color_ == ColorConstant::electric_blue) {
        fade_color_anim_.reverse(sprite_, dt);
        head_.set_mix(sprite_.get_mix());
    } else {
        fade_color_anim_.advance(sprite_, dt);
        head_.set_mix(sprite_.get_mix());
    }
}


void Wanderer::injured(Platform& pf, Game& game, Health amount)
{
    if (state_ == State::sleep) {
        return;
    }

    const bool was_second_form = second_form(game);

    if (sprite_.get_mix().amount_ < 180) {
        pf.sleep(2);
    }

    damage_ += amount;

    debit_health(pf, amount);

    if (alive()) {
        pf.speaker().play_sound("click", 1, position_);
    }

    if (not was_second_form and second_form(game)) {
        game.camera().shake();

        medium_explosion(pf, game, position_);
    }

    if (state_ not_eq State::big_laser_shooting and
        state_ not_eq State::big_laser1) {

        const auto c = current_zone(game).injury_glow_color_;

        sprite_.set_mix({c, 255});
        head_.set_mix({c, 255});
    }

    show_boss_health(
        pf, game, 0, Float(get_health()) / get_initial_health(game));
}


void Wanderer::on_collision(Platform& pf, Game& game, Laser&)
{
    injured(pf, game, Health{1});
}


void Wanderer::on_collision(Platform& pf, Game& game, LaserExplosion&)
{
    injured(pf, game, Health{8});
}


void Wanderer::on_collision(Platform& pf, Game& game, AlliedOrbShot&)
{
    if (not is_allied()) {
        injured(pf, game, Health{1});
    }
}


void Wanderer::on_death(Platform& pf, Game& game)
{
    boss_explosion(pf, game, position_);
}


void Wanderer::sync(const net_event::EnemyStateSync& s, Game& game)
{
    state_ = static_cast<State>(s.state_);
    timer_ = 0;
    position_.x = s.x_.get();
    position_.y = s.y_.get();

    sprite_.set_position(position_);
    head_.set_position({position_.x, position_.y - 16});
    shadow_.set_position(position_);
}
