#include "gatekeeper.hpp"
#include "blind_jump/entity/enemies/common.hpp"
#include "blind_jump/game.hpp"
#include "boss.hpp"
#include "number/random.hpp"


class ScattershotAttackPattern : public GatekeeperShield::AttackPattern {
public:
    ScattershotAttackPattern(Microseconds delay, Game& game, Gatekeeper& gk)
        : reload_(delay), shot_count_(0)
    {
        switch (game.difficulty()) {
        case Settings::Difficulty::easy:
            reload_ += milliseconds(600);
            break;

        case Settings::Difficulty::count:
        case Settings::Difficulty::normal:
            reload_ += milliseconds(300);
            break;

        case Settings::Difficulty::hard:
        case Settings::Difficulty::survival:
            break;
        }
    }

    void update(GatekeeperShield& shield,
                Platform& pfrm,
                Game& game,
                Microseconds dt)
    {
        if (reload_ > 0) {
            reload_ -= dt;
        }

        if (reload_ <= 0) {

            reload_ = milliseconds(40 + shot_count_ * 10);

            shield.shoot(pfrm,
                         game,
                         shield.get_position(),
                         rng::sample<22>(shield.get_target(game).get_position(),
                                         rng::critical_state),
                         0.00019f - 0.00002f * shot_count_,
                         seconds(2) - milliseconds(550));

            if (++shot_count_ == [&] { return 3; }()) {
                shot_count_ = 0;
                switch (game.difficulty()) {
                case Settings::Difficulty::easy:
                    reload_ = milliseconds(1900);
                    break;

                case Settings::Difficulty::count:
                case Settings::Difficulty::normal:
                    reload_ = milliseconds(1300);
                    break;

                case Settings::Difficulty::hard:
                case Settings::Difficulty::survival:
                    reload_ = milliseconds(900);
                    break;
                }
            }
        }
    }

private:
    Microseconds reload_;
    u8 shot_count_;
};


class SweepAttackPattern : public GatekeeperShield::AttackPattern {
public:
    SweepAttackPattern(GatekeeperShield& shield, Microseconds delay, Game& game)
        : reload_(delay), rot_(-60),
          target_(shield.get_target(game).get_position())
    {
        const auto& shield_pos = shield.get_position();
        if (target_.x < shield_pos.x) {
            spin_ = true;
            rot_ = 60;
        } else {
            spin_ = false;
        }
    }

    void update(GatekeeperShield& shield,
                Platform& pfrm,
                Game& game,
                Microseconds dt)
    {
        if (reload_ > 0) {
            reload_ -= dt;
        }

        if (reload_ <= 0) {
            reload_ = milliseconds(280);


            const auto interval = [&] {
                switch (game.difficulty()) {
                case Settings::Difficulty::easy:
                    return 35;

                case Settings::Difficulty::count:
                case Settings::Difficulty::normal:
                    return 22;

                case Settings::Difficulty::hard:
                case Settings::Difficulty::survival:
                    break;
                }
                return 19;
            }();


            if (not spin_) {
                rot_ += interval;
            } else {
                rot_ -= interval;
            }

            auto angle = rot_;
            if (angle < 0) {
                angle = 360 + angle;
            }

            angle %= 360;

            if (auto shot =
                    shield.shoot(pfrm,
                                 game,
                                 shield.get_position(),
                                 rng::sample<4>(target_, rng::critical_state),
                                 0.00013f,
                                 seconds(3) + milliseconds(500))) {
                shot->rotate(angle);
            }
        }
    }

private:
    Microseconds reload_;
    int rot_;
    bool spin_;
    Vec2<Float> target_;
};


static const Entity::Health initial_health = 125;
static const int default_shield_radius = 24;
static const int max_shield_radius = 100;


static const char* boss_music = "omega";


static Float shield_rotation_multiplier(Gatekeeper& g, int radius)
{
    const auto result =
        (1.f - smoothstep(0.f, 1.f, Float(radius) / max_shield_radius));

    if (not g.third_form()) {
        return result * 0.67f;
    }
    return result;
}


static void set_shield_speed(Float multiplier, Game& game)
{
    for (auto& shield : game.enemies().get<GatekeeperShield>()) {
        shield->set_speed(multiplier);
    }
}


Gatekeeper::Gatekeeper(const Vec2<Float>& position, Game& game)
    : Enemy(initial_health, position, {{28, 50}, {16, 36}}),
      state_(State::sleep), timer_(0), charge_timer_(0),
      shield_radius_(default_shield_radius)
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

    game.enemies().spawn<GatekeeperShield>(position, 0);
    game.enemies().spawn<GatekeeperShield>(position, INT16_MAX / 2);

    set_shield_speed(shield_rotation_multiplier(*this, shield_radius_), game);
}


void Gatekeeper::update(Platform& pfrm, Game& game, Microseconds dt)
{
    if (UNLIKELY(is_allied_)) {
        is_allied_ = false;
    }
    Enemy::update(pfrm, game, dt);

    static constexpr const Microseconds jump_duration = milliseconds(500);

    Float movement_rate = 0.000033f;
    if (game.difficulty() == Settings::Difficulty::easy) {
        movement_rate = 0.000026f;
    }

    charge_timer_ += dt;

    auto face_left = [this] {
        sprite_.set_flip({1, 0});
        head_.set_flip({1, 0});
    };

    auto face_right = [this] {
        sprite_.set_flip({0, 0});
        head_.set_flip({0, 0});
    };

    auto face_player =
        [this, &player = game.player(), &face_left, &face_right] {
            if (player.get_position().x > position_.x) {
                face_left();
            } else {
                face_right();
            }
        };

    auto dist_from_player = [&](Float dist) {
        return distance(position_, game.player().get_position()) > dist;
    };

    switch (state_) {
    case State::sleep:
        if (visible()) {
            // Start facing away from the player
            if (game.player().get_position().x > position_.x) {
                face_left();
            } else {
                face_right();
            }

            timer_ += dt;
            if (timer_ > seconds(1)) {
                state_ = State::idle;
                timer_ = 0;

                pfrm.speaker().play_music(boss_music, 0);

                show_boss_health(
                    pfrm, game, 0, Float(get_health()) / initial_health);
            }
        }
        break;

    case State::second_form_enter:
        timer_ += dt;
        if (timer_ > milliseconds(25)) {
            if (shield_radius_ < max_shield_radius + 90) {
                ++shield_radius_;
                set_shield_speed(0.6f, game);
            } else {
                state_ = State::encircle_receede;
                set_shield_speed(1.f, game);
                timer_ = 0;
            }
        }
        break;

    case State::third_form_enter:
        state_ = State::third_form_sweep_in;
        timer_ = 0;
        break;

    case State::third_form_sweep_in:
        timer_ += dt;
        if (timer_ > milliseconds(25)) {
            timer_ = 0;

            if (shield_radius_ > default_shield_radius) {
                --shield_radius_;

                set_shield_speed(
                    shield_rotation_multiplier(*this, shield_radius_), game);

            } else {
                state_ = State::idle;
            }
        }
        break;


    case State::idle:
        face_player();

        timer_ += dt;
        if (timer_ > [&]() {
                if (third_form()) {
                    return milliseconds(470);
                } else if (second_form()) {
                    return milliseconds(500);
                } else {
                    return milliseconds(600);
                }
            }()) {
            timer_ = 0;

            if (second_form() or dist_from_player(80)) {
                state_ = State::jump;

                sprite_.set_texture_index(48);
                head_.set_texture_index(42);

            } else {
                if (not second_form()) {
                    if (charge_timer_ > seconds(9)) {
                        charge_timer_ = 0;
                        state_ = State::prep_shield_sweep;

                        sprite_.set_texture_index(12);
                        head_.set_texture_index(10);
                    }
                }
            }
        }
        break;

    case State::prep_shield_sweep:
        timer_ += dt;
        if (timer_ > milliseconds(50)) {
            sprite_.set_texture_index(13);
            head_.set_texture_index(11);

            timer_ = 0;
            state_ = State::shield_sweep_out;
        }
        break;

    case State::encircle_receede:
        timer_ += dt;
        if (timer_ > milliseconds(25)) {
            timer_ = 0;

            if (shield_radius_ > max_shield_radius + 10) {
                --shield_radius_;
            } else {
                state_ = State::idle;
            }
        }
        break;

    case State::jump:
        timer_ += dt;
        if (timer_ > milliseconds(90)) {
            timer_ = 0;

            const auto index = head_.get_texture_index();
            if (index < 46) {
                head_.set_texture_index(index + 1);
                sprite_.set_texture_index(sprite_.get_texture_index() + 1);

            } else {
                state_ = State::airborne;

                Vec2<Float> dest;
                Vec2<Float> unit;

                int tries = 0;

                do {

                    if (tries++ > 0) {
                        const s16 dir = ((static_cast<float>(rng::choice<359>(
                                             rng::critical_state))) /
                                         360) *
                                        INT16_MAX;

                        unit = {(float(cosine(dir)) / INT16_MAX),
                                (float(sine(dir)) / INT16_MAX)};

                    } else {
                        unit =
                            direction(position_, game.player().get_position());
                    }

                    move_vec_ = 5.f * unit;

                    const auto tolerance = milliseconds(90);
                    dest = position_ +
                           move_vec_ *
                               ((jump_duration + tolerance) * movement_rate);

                } while (wall_in_path(unit, position_, game, dest));
            }
        }
        break;

    case State::airborne: {
        timer_ += dt;

        const Float offset = 10 *
                             Float(sine(4 * 3.14f * 0.0027f * timer_ + 180)) /
                             std::numeric_limits<s16>::max();

        position_.x += move_vec_.x * dt * movement_rate;
        position_.y += move_vec_.y * dt * movement_rate;

        sprite_.set_position({position_.x, position_.y - abs(offset)});
        head_.set_position({position_.x, position_.y - abs(offset)});
        shadow_.set_position(position_);

        if (timer_ > milliseconds(300)) {
            sprite_.set_texture_index(53);
            head_.set_texture_index(47);
        }
        if (timer_ > jump_duration) {
            head_.set_position(position_);
            sprite_.set_position(position_);

            head_.set_texture_index(45);
            sprite_.set_texture_index(45 + 6);
            state_ = State::landing;
            game.camera().shake();
            game.rumble(pfrm, milliseconds(125));
        }
        break;
    }

    case State::landing:
        timer_ += dt;
        if (timer_ > milliseconds(90)) {
            timer_ = 0;

            const auto index = head_.get_texture_index();
            if (index > 42) {
                head_.set_texture_index(index - 1);
                sprite_.set_texture_index(sprite_.get_texture_index() - 1);

            } else {
                state_ = State::idle;
                sprite_.set_texture_index(7);
                head_.set_texture_index(6);
            }
        }
        break;

    case State::shield_sweep_out:
        face_player();
        timer_ += dt;

        if (timer_ > milliseconds(third_form() ? 35 : 40)) {
            timer_ = 0;
            if (shield_radius_ < max_shield_radius) {
                ++shield_radius_;

                if (shield_radius_ == max_shield_radius - 30) {
                    game.camera().shake(6);
                    Buffer<GatekeeperShield*, 2> shields;
                    for (auto& shield :
                         game.enemies().get<GatekeeperShield>()) {
                        shield->open();
                        shields.push_back(shield.get());
                    }
                    std::sort(
                        shields.begin(),
                        shields.end(),
                        [&](GatekeeperShield* lhs, GatekeeperShield* rhs) {
                            return distance(lhs->get_position(),
                                            game.player().get_position()) >
                                   distance(rhs->get_position(),
                                            game.player().get_position());
                        });
                    if (shields.size() == 2) {
                        shields[0]
                            ->set_attack_pattern<ScattershotAttackPattern>(
                                milliseconds(700), game, *this);
                        shields[1]->set_attack_pattern<SweepAttackPattern>(
                            *shields[1], milliseconds(100), game);
                    } else if (shields.size() == 1) {
                        shields[0]
                            ->set_attack_pattern<ScattershotAttackPattern>(
                                milliseconds(700), game, *this);
                    }
                }

                set_shield_speed(
                    shield_rotation_multiplier(*this, shield_radius_), game);

            } else {
                state_ = State::shield_sweep_in1;
                sprite_.set_texture_index(7);
                head_.set_texture_index(6);
            }
        }
        break;

    case State::shield_sweep_in1:
        face_player();
        timer_ += dt;

        if (timer_ > milliseconds(third_form() ? 42 : 45)) {
            timer_ = 0;
            if (shield_radius_ > max_shield_radius / 2) {
                --shield_radius_;

                if (shield_radius_ == max_shield_radius - 30) {
                    for (auto& shield :
                         game.enemies().get<GatekeeperShield>()) {
                        shield->close();
                    }
                }

                set_shield_speed(
                    0.5f * shield_rotation_multiplier(*this, shield_radius_),
                    game);

            } else {
                state_ = State::shield_sweep_in2;
            }
        }
        break;

    case State::shield_sweep_in2:
        timer_ += dt;
        if (timer_ > milliseconds(60)) {
            timer_ = 0;
            if (shield_radius_ > default_shield_radius) {
                --shield_radius_;

                set_shield_speed(
                    0.75f * shield_rotation_multiplier(*this, shield_radius_),
                    game);

            } else {
                set_shield_speed(
                    shield_rotation_multiplier(*this, shield_radius_), game);

                state_ = State::idle;
            }
        }
        break;
    }


    fade_color_anim_.advance(sprite_, dt);
    head_.set_mix(sprite_.get_mix());
}


void Gatekeeper::injured(Platform& pfrm, Game& game, Health amount)
{
    if (sprite_.get_mix().amount_ < 180) {
        pfrm.sleep(2);
    }

    damage_ += amount;

    const bool was_second_form = second_form();
    const bool was_third_form = third_form();
    debit_health(pfrm, amount);

    if (alive()) {
        pfrm.speaker().play_sound("click", 1, position_);
    }

    if (not was_second_form and second_form()) {
        state_ = State::second_form_enter;
        timer_ = 0;
        sprite_.set_position(position_);
        sprite_.set_texture_index(7);
        head_.set_position(position_);
        head_.set_texture_index(6);

        game.camera().shake();
        medium_explosion(pfrm, game, position_);

        for (auto& shield : game.enemies().get<GatekeeperShield>()) {
            shield->close();
            shield->make_allied(false);
        }

    } else if (not was_third_form and third_form()) {
        state_ = State::third_form_enter;
        timer_ = 0;
        sprite_.set_position(position_);
        sprite_.set_texture_index(7);
        head_.set_position(position_);
        head_.set_texture_index(6);

        game.camera().shake();
        medium_explosion(pfrm, game, position_);

        for (auto& shield : game.enemies().get<GatekeeperShield>()) {
            shield->close();
            shield->make_allied(false);
        }
    }

    const auto c = current_zone(game).injury_glow_color_;

    sprite_.set_mix({c, 255});
    head_.set_mix({c, 255});

    show_boss_health(pfrm, game, 0, Float(get_health()) / initial_health);
}


void Gatekeeper::on_collision(Platform& pfrm, Game& game, Laser&)
{
    injured(pfrm, game, Health{1});
}


void Gatekeeper::on_collision(Platform& pfrm, Game& game, LaserExplosion&)
{
    injured(pfrm, game, Health{8});
}


void Gatekeeper::on_collision(Platform& pfrm, Game& game, AlliedOrbShot&)
{
    if (not is_allied()) {
        injured(pfrm, game, Health{1});
    }
}


void Gatekeeper::on_death(Platform& pfrm, Game& game)
{
    auto loc = position_ + sprite_.get_origin().cast<Float>();
    loc.x -= 16;
    boss_explosion(pfrm, game, loc);
}


GatekeeperShield::GatekeeperShield(const Vec2<Float>& position, int offset)
    : Enemy(Health(30), position, {{14, 32}, {8, 16}}), timer_(0),
      shadow_update_timer_(0), detached_(false), opened_(false),
      offset_(offset), speed_(1.f)
{
    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_origin({8, -11});

    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_texture_index(16);
    sprite_.set_origin({8, 16});
}


void GatekeeperShield::detach(Microseconds keepalive)
{
    timer_ = keepalive;
    detached_ = true;

    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_texture_index(33);
}


void GatekeeperShield::open()
{
    opened_ = true;
    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_texture_index(33);
    sprite_.set_origin({16, 16});
}


void GatekeeperShield::close()
{
    sprite_.set_origin({8, 16});
    opened_ = false;
}


void GatekeeperShield::update(Platform& pfrm, Game& game, Microseconds dt)
{
    Enemy::update(pfrm, game, dt);

    fade_color_anim_.advance(sprite_, dt);

    if (not detached_ and game.enemies().get<Gatekeeper>().empty()) {
        this->detach(seconds(3) +
                     milliseconds(rng::choice<900>(rng::utility_state)));
    }

    if (detached_) {
        timer_ -= dt;
        if (timer_ <= 0) {
            if (timer_ <= 0) {
                this->kill();
            }
        }
        return;
    }

    if (opened_) {
        attack(pfrm, game, dt);
    }

    shadow_update_timer_ += dt;

    if (shadow_update_timer_ > milliseconds(200)) {
        shadow_update_timer_ = 0;

        auto coord = to_tile_coord(position_.cast<s32>());
        coord.y += 1;

        // Hide the shadow when we're jumping over empty space
        if (not is_walkable(game.tiles().get_tile(coord.x, coord.y))) {
            shadow_.set_alpha(Sprite::Alpha::transparent);
        } else {
            shadow_.set_alpha(Sprite::Alpha::translucent);
        }
    }

    auto set_flip = [this](Float x_part, Float y_part) {
        // Rather than draw additional artwork and waste vram
        // for a full rotation animation, which is relatively
        // simple, I animated a quarter of the shield rotation,
        // and mirrored the sprite accordingly.
        if (y_part > 0) {
            if (x_part > 0) {
                sprite_.set_flip({true, false});
            } else {
                sprite_.set_flip({false, false});
            }
        } else {
            if (x_part > 0) {
                sprite_.set_flip({false, false});
            } else {
                sprite_.set_flip({true, false});
            }
        }
    };

    auto set_keyframe = [this](Float y_part) {
        if (abs(y_part) > 0.8f) {
            sprite_.set_size(Sprite::Size::w16_h32);
            sprite_.set_texture_index(16);

        } else if (abs(y_part) > 0.6f) {
            sprite_.set_size(Sprite::Size::w16_h32);
            sprite_.set_texture_index(17);

        } else if (abs(y_part) > 0.4f) {
            sprite_.set_size(Sprite::Size::w16_h32);
            sprite_.set_texture_index(18);

        } else {
            sprite_.set_size(Sprite::Size::w16_h32);
            sprite_.set_texture_index(19);
        }
    };

    timer_ += (dt / 32) * speed_;

    const auto x_part = (Float(cosine(timer_ + offset_)) / INT16_MAX);
    const auto y_part = (Float(sine(timer_ + offset_)) / INT16_MAX);

    if (not opened_) {
        set_keyframe(y_part);
        set_flip(x_part, y_part);
    }

    const auto& parent_pos =
        (*game.enemies().get<Gatekeeper>().begin())->get_position();

    const auto radius =
        (*game.enemies().get<Gatekeeper>().begin())->shield_radius();

    position_ = {parent_pos.x + radius * x_part,
                 (parent_pos.y - 2) + radius * y_part};

    sprite_.set_position(position_);
    shadow_.set_position(position_);
}


void GatekeeperShield::on_collision(Platform& pfrm, Game& game, Laser&)
{
    if (sprite_.get_size() == Sprite::Size::w32_h32) {
        const auto c = current_zone(game).injury_glow_color_;
        sprite_.set_mix({c, 255});
        debit_health(pfrm, 1); // So, technically you can destroy the shield,
                               // but it's probably not worth the trouble of
                               // trying.

    } else {
        const auto c = current_zone(game).energy_glow_color_;
        sprite_.set_mix({c, 255});
    }
}


void GatekeeperShield::on_collision(Platform& pfrm, Game& game, AlliedOrbShot&)
{
    if (not is_allied()) {
        if (sprite_.get_size() == Sprite::Size::w32_h32) {
            const auto c = current_zone(game).injury_glow_color_;
            sprite_.set_mix({c, 255});
            debit_health(pfrm, 1); // So, technically you can destroy the
                                   // shield, but it's probably not worth the
                                   // trouble of trying.

        } else {
            const auto c = current_zone(game).energy_glow_color_;
            sprite_.set_mix({c, 255});
        }
    }
}


void GatekeeperShield::on_death(Platform& pfrm, Game& game)
{
    pfrm.sleep(5);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::null};

    on_enemy_destroyed(pfrm, game, 0, position_, 1, item_drop_vec);
}


GatekeeperShield::~GatekeeperShield()
{
    if (attack_pattern_set_) {
        ((AttackPattern*)attack_pattern_)->~AttackPattern();
    }
}


void GatekeeperShield::attack(Platform& pfrm, Game& game, Microseconds dt)
{
    if (not opened_) {
        return;
    }

    if (attack_pattern_set_) {
        ((AttackPattern*)attack_pattern_)->update(*this, pfrm, game, dt);
    }
}
