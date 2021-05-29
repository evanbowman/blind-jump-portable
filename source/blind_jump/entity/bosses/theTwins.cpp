#include "theTwins.hpp"
#include "blind_jump/game.hpp"
#include "boss.hpp"


static const Entity::Health initial_health = 58;


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
        shadow_.set_position({position_.x + shadow_offset_, position_.y});
        shadow2_.set_position({position_.x + shadow_offset_, position_.y});
        hitbox_.dimension_.origin_ = {16, 8};
    } else {
        head_.set_position({position_.x + 18, position_.y});
        sprite_.set_position({position_.x - 14, position_.y});
        shadow_.set_position({position_.x - shadow_offset_, position_.y});
        shadow2_.set_position({position_.x - shadow_offset_, position_.y});
        hitbox_.dimension_.origin_ = {28, 8};
    }
}


Twin::Twin(const Vec2<Float>& position)
    : Enemy(initial_health, position, {{40, 14}, {0, 0}})
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

    helper_.sprite_.set_size(Sprite::Size::w16_h32);
    helper_.sprite_.set_origin({8, 0});

    set_sprite(6);
}


void Twin::Helper::update(Platform& pf,
                          Game& game,
                          Microseconds dt,
                          const Entity& parent,
                          const Entity& target)
{
    const auto parent_pos = parent.get_position();

    Vec2<Float> target_pos = {parent.get_sprite().get_flip().x
                                  ? parent_pos.x - 15
                                  : parent_pos.x + 15,
                              parent_pos.y - 34};

    Float interp_speed = 0.000006f;
    if (state_ == State::recharge) {
        target_ = target_pos;
        interp_speed = 0.0000028f;
    }

    sprite_.set_position(
        interpolate(target_, sprite_.get_position(), dt * interp_speed));

    if (target.get_position().x > sprite_.get_position().x) {
        sprite_.set_flip({true, false});
    } else {
        sprite_.set_flip({false, false});
    }

    timer_ += dt;

    auto shoot_pos = sprite_.get_position();
    shoot_pos.y += 16;

    switch (state_) {
    case Helper::State::recharge:
        if (timer_ > [&] {
                if (game.difficulty() == Settings::Difficulty::easy) {
                    return seconds(2) + milliseconds(250);
                } else {
                    return seconds(1) + milliseconds(500);
                }
            }()) {
            timer_ = 0;
            state_ = Helper::State::shoot1;
        }
        break;

    case Helper::State::shoot1:
        if (timer_ > milliseconds(150)) {
            timer_ = 0;
            pf.speaker().play_sound("laser1", 4, sprite_.get_position());
            if (game.effects().spawn<OrbShot>(
                    shoot_pos, target.get_position(), 0.00015f, seconds(2))) {
            }
            auto pos = sprite_.get_position();
            pos.x -= sprite_.get_flip().x ? 2 : -2;
            sprite_.set_position(pos);
            state_ = Helper::State::shoot2;
        }
        break;

    case Helper::State::shoot2:
        if (timer_ > milliseconds(150)) {
            timer_ = 0;
            pf.speaker().play_sound("laser1", 4, sprite_.get_position());
            if (game.effects().spawn<OrbShot>(
                    shoot_pos, target.get_position(), 0.00015f, seconds(2))) {
            }
            auto pos = sprite_.get_position();
            pos.x -= sprite_.get_flip().x ? 2 : -2;
            sprite_.set_position(pos);
            state_ = Helper::State::shoot3;
        }
        break;

    case Helper::State::shoot3:
        if (timer_ > milliseconds(150)) {
            timer_ = 0;
            pf.speaker().play_sound("laser1", 4, sprite_.get_position());
            if (game.effects().spawn<OrbShot>(
                    shoot_pos, target.get_position(), 0.00015f, seconds(2))) {
            }
            auto pos = sprite_.get_position();
            pos.x -= sprite_.get_flip().x ? 2 : -2;
            sprite_.set_position(pos);
            state_ = Helper::State::recharge;
        }
        break;
    }
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

    if (helper_.sprite_.get_texture_index() == 65) {
        helper_.update(pf, game, dt, *this, target);
    }

    auto shoot_offset = [&]() -> Vec2<Float> {
        if (sprite_.get_flip().x) {
            return {22, -5};
        } else {
            return {-22, -5};
        }
    };

    const auto second_form = [&] { return get_health() < initial_health / 2; };

    constexpr Microseconds leap_duration = milliseconds(120);
    const Float movement_rate = [&] {
        if (second_form()) {
            switch (game.difficulty()) {
            case Settings::Difficulty::easy:
                return 0.000035f;

            case Settings::Difficulty::count:
            case Settings::Difficulty::normal:
                break;

            case Settings::Difficulty::survival:
            case Settings::Difficulty::hard:
                return 0.000041f;
            }
            return 0.000039f;
        } else {
            switch (game.difficulty()) {
            case Settings::Difficulty::easy:
                return 0.0000305f;

            case Settings::Difficulty::count:
            case Settings::Difficulty::normal:
                break;

            case Settings::Difficulty::survival:
            case Settings::Difficulty::hard:
                return 0.0000345f;
            }
            return 0.0000335f;
        }
    }();
    constexpr Microseconds mode2_move_duration = milliseconds(140);
    const Float mode2_move_rate = [&] {
        switch (game.difficulty()) {
        case Settings::Difficulty::easy:
            return 0.0000405f;

        case Settings::Difficulty::count:
        case Settings::Difficulty::normal:
            break;

        case Settings::Difficulty::survival:
        case Settings::Difficulty::hard:
            return 0.000045f;
        }
        return 0.000044f;
    }();

    switch (state_) {
    case State::inactive:
        face_target();

        if (visible()) {
            timer_ += dt;
            if (timer_ > seconds(2)) {
                if (auto s = sibling(game)) {
                    if (s->id() < id()) {
                        pf.speaker().play_music("omega", 0);
                    }
                }
                state_ = State::idle;
                timer_ = 0;
            }
        }
        break;

    case State::prep_mutation:
        // Just to make sure that we're a focal point onscreen for the
        // transition animation.
        game.camera().push_ballast(position_);

        if (sprite_.get_mix().amount_ == 0) {
            set_health(std::max(get_health(), Health(20)));
            state_ = State::mutate;
            pf.load_sprite_texture("spritesheet_boss2_mutate");
        }
        break;

    case State::mutate:
        game.camera().push_ballast(position_);
        timer_ += dt;
        if (timer_ > milliseconds(100)) {
            shadow_offset_ = -2;
            timer_ = 0;
            auto index = sprite_.get_texture_index();
            if (index == 6) {
                set_sprite(41);
            } else if (index < 49) {
                set_sprite(index + 2);
            } else {
                state_ = State::mutate_done;
                pf.load_sprite_texture("spritesheet_boss2_final");
                helper_.sprite_.set_texture_index(65);
            }
        }
        break;

    case State::mutate_done:
        pf.speaker().play_music("omega", 0);
        state_ = State::mode2_idle;
        break;

    case State::mode2_long_idle:
        timer_ += dt;
        face_target();
        if (timer_ > seconds(2)) {
            timer_ = 0;
            state_ = State::mode2_idle;
            if (rng::choice<2>(rng::critical_state) == 0) {

                state_ = State::mode2_open_mouth;

                set_sprite(55);
                medium_explosion(
                    pf, game, sprite_.get_position() + shoot_offset());
                game.camera().shake();
            }
        }
        break;

    case State::mode2_open_mouth:
        timer_ += dt;
        if (timer_ > milliseconds(150)) {
            timer_ = 0;
            state_ = State::mode2_ranged_attack_charge;
        }
        break;

    case State::mode2_ranged_attack_charge:
        if (timer_ < seconds(1) and timer_ + dt > seconds(1)) {
            sprite_.set_mix({current_zone(game).energy_glow_color_, 0});
        }
        timer_ += dt;
        alt_timer_ += dt;
        if (alt_timer_ > milliseconds(80)) {
            alt_timer_ = 0;
            if (sprite_.get_texture_index() == 55) {
                sprite_.set_texture_index(57);
                alt_timer_ = -milliseconds(30);
            } else {
                sprite_.set_texture_index(55);
            }
        }
        if (timer_ > seconds(2) - milliseconds(100)) {
            timer_ = 0;
            alt_timer_ = 0;
            state_ = State::mode2_ranged_attack;
            sprite_.set_texture_index(55);
            medium_explosion(pf, game, sprite_.get_position() + shoot_offset());
            game.camera().shake();
            target_ = target.get_position();
        }
        break;

    case State::mode2_ranged_attack:
        timer_ += dt;
        alt_timer_ += dt;

        target_ = interpolate(target.get_position(), target_, dt * 0.000020f);

        if (alt_timer_ > [&] {
                if (game.difficulty() == Settings::Difficulty::easy) {
                    return milliseconds(800);
                } else if (game.difficulty() == Settings::Difficulty::normal) {
                    return milliseconds(640);
                } else {
                    return milliseconds(520);
                }
            }()) {
            alt_timer_ = 0;

            game.effects().spawn<ConglomerateShot>(
                position_ + shoot_offset(),
                rng::sample<32>(target_, rng::critical_state),
                0.00011f);
        }
        if (timer_ > seconds(3)) {
            state_ = State::mode2_ranged_attack_done;
        }
        break;

    case State::mode2_ranged_attack_done:
        state_ = State::mode2_idle;
        break;

    case State::mode2_idle:
        timer_ += dt;
        if (timer_ > milliseconds(150)) {
            timer_ = 0;
            show_boss_health(pf, game, 0, Float(get_health()) / initial_health);
            face_target();
            state_ = State::mode2_prep_move;
        }
        break;

    case State::mode2_prep_move: {
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
            dest = position_ + speed_ * ((mode2_move_duration + tolerance) *
                                         mode2_move_rate);

        } while (wall_in_path(unit, position_, game, dest));
        leaps_ += 1;
        state_ = State::mode2_start_move;
        set_sprite(6);
        break;
    }

    case State::mode2_start_move:
        timer_ += dt;
        position_.x += speed_.x * dt * mode2_move_rate * 0.25f;
        position_.y += speed_.y * dt * mode2_move_rate * 0.25f;

        if (timer_ > milliseconds(60)) {
            timer_ = 0;
            auto index = sprite_.get_texture_index();
            if (index < 14) {
                set_sprite(index + 2);
            } else {
                state_ = State::mode2_moving;
            }
            if (index == 12) {
                shadow_offset_ = 0;
            } else if (index == 14) {
                shadow_offset_ = 2;
            }
        }
        break;

    case State::mode2_moving:
        timer_ += dt;
        position_.x += speed_.x * dt * mode2_move_rate;
        position_.y += speed_.y * dt * mode2_move_rate;

        if (timer_ > mode2_move_duration) {
            state_ = State::mode2_stop_move;
            timer_ = 0;
            set_sprite(41);
            // set_sprite(49);
        }
        break;

    case State::mode2_stop_move: {
        timer_ += dt;
        if (timer_ > milliseconds(60)) {
            timer_ = 0;
            auto index = sprite_.get_texture_index();
            if (index < 53) {
                if (index + 2 == 49) {
                    set_sprite(index + 4);
                } else {
                    set_sprite(index + 2);
                }
                if (index == 41) {
                    shadow_offset_ = 1;
                } else if (index == 43) {
                    shadow_offset_ = 0;
                } else if (index == 45) {
                    shadow_offset_ = -1;
                } else if (index == 51) {
                    shadow_offset_ = -2;
                }
            } else {
                set_sprite(49);
                state_ = State::mode2_stop_friction;
            }
        }
        speed_ = interpolate(Vec2<Float>{0}, speed_, dt * 0.000007f);

        position_.x += speed_.x * dt * movement_rate;
        position_.y += speed_.y * dt * movement_rate;
        break;
    }

    case State::mode2_stop_friction:
        timer_ += dt;
        if (timer_ > milliseconds(100)) {
            timer_ = 0;
            speed_ = {};
            if (leaps_ > 4) {
                leaps_ = 0;
                state_ = State::mode2_long_idle;
            } else {
                state_ = State::mode2_idle;
            }
        }
        speed_ = interpolate(Vec2<Float>{0}, speed_, dt * 0.000016f);

        position_.x += speed_.x * dt * movement_rate;
        position_.y += speed_.y * dt * movement_rate;
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
        if (sibling(game) == nullptr) {
            if (not visible()) {
                break;
            }
            timer_ = 0;
            state_ = State::prep_mutation;
            sprite_.set_mix({ColorConstant::spanish_crimson, 255});
            set_sprite(6);
            break;
        }
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

        if (alt_timer_ > milliseconds(250)) {
            alt_timer_ = 0;

            const Float atk_mov_rate = [&] {
                if (second_form()) {
                    return 0.00016f;
                } else {
                    return 0.00013f;
                }
            }();

            game.effects().spawn<WandererSmallLaser>(
                position_ + shoot_offset(), target_, atk_mov_rate);

            if (second_form()) {

                const auto incidence =
                    interpolate(27, 70, Float(timer_) / seconds(3));

                if (game.effects().spawn<WandererSmallLaser>(
                        position_ + shoot_offset(), target_, atk_mov_rate)) {
                    (*game.effects().get<WandererSmallLaser>().begin())
                        ->rotate(incidence);
                }

                if (game.effects().spawn<WandererSmallLaser>(
                        position_ + shoot_offset(), target_, atk_mov_rate)) {
                    (*game.effects().get<WandererSmallLaser>().begin())
                        ->rotate(360 - incidence);
                }
            }
        }


        if (timer_ > [&] {
                switch (game.difficulty()) {
                case Settings::Difficulty::easy:
                    return seconds(3) - milliseconds(700);

                case Settings::Difficulty::count:
                case Settings::Difficulty::normal:
                    break;

                case Settings::Difficulty::survival:
                case Settings::Difficulty::hard:
                    return seconds(3) + milliseconds(250);
                }
                return seconds(3);
            }()) {
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

        } while (abs(speed_.x) < 1 // The dashing animation just looks
                                   // strange when the character is moving
                                   // vertically.
                 and wall_in_path(unit, position_, game, dest));
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
    if (sprite_.get_mix().amount_ < 180) {
        pfrm.sleep(2);
    }

    if (alive()) {
        pfrm.speaker().play_sound("click", 1, position_);
    }

    damage_ += amount;

    debit_health(pfrm, amount);

    if (auto s = sibling(game)) {
        show_boss_health(
            pfrm, game, s->id() < id(), Float(get_health()) / initial_health);
    } else {
        show_boss_health(pfrm, game, 0, Float(get_health()) / initial_health);
    }

    if (state_ not_eq State::ranged_attack_charge and
        state_ not_eq State::prep_mutation and
        state_ not_eq State::mode2_ranged_attack_charge) {

        const auto c = current_zone(game).injury_glow_color_;
        sprite_.set_mix({c, 255});
        head_.set_mix({c, 255});
    }
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

    auto make_scraps = [&] {
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

    if (sibling(game) == nullptr) {
        sprite_.set_alpha(Sprite::Alpha::transparent);
        head_.set_alpha(Sprite::Alpha::transparent);
        pf.load_sprite_texture("spritesheet_boss2_done");
        return;
    }

    pf.speaker().play_sound("explosion1", 3, position_);

    for (int i = 0; i < 2; ++i) {
        game.details().spawn<Item>(
            rng::sample<32>(position_, rng::utility_state),
            pf,
            Item::Type::heart);
    }

    if (locale_language_name(locale_get_language()) == "english") {
        push_notification(pf, game.state(), "Twin defeated...");
    }

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

    game.camera().shake(18);
}
