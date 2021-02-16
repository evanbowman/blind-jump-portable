#include "infestedCore.hpp"
#include "blind_jump/game.hpp"
#include "boss.hpp"


static const Entity::Health initial_health(60);


InfestedCore::InfestedCore(const Vec2<Float>& position)
    : Enemy(initial_health, position, {{32, 40}, {16, 32}}),
      state_(State::sleep), anim_timer_(0), spawn_timer_(0)
{
    set_position({position.x + 16, position.y + 4});

    sprite_.set_texture_index(44);
    sprite_.set_position(position_);
    sprite_.set_origin({16, 16});

    top_.set_texture_index(50);
    top_.set_position({position_.x, position_.y - 32});
    top_.set_origin({16, 16});

    shadow_.set_origin({16, -2});
    shadow_.set_texture_index(49);
    shadow_.set_position(position_);
    shadow_.set_alpha(Sprite::Alpha::translucent);
}


void InfestedCore::update(Platform& pfrm, Game& game, Microseconds dt)
{
    anim_timer_ += dt;

    auto animate = [&] {
        if (sprite_.get_texture_index() == 48) {
            return;
        }
        if (anim_timer_ >= milliseconds(150)) {
            anim_timer_ = 0;

            const auto idx = sprite_.get_texture_index();
            if (idx < 47) {
                sprite_.set_texture_index(idx + 1);
            } else {
                sprite_.set_texture_index(44);
            }
        }
    };

    auto enemy_spawn_coord = [&] {
        s8 x;
        s8 y;

        do {
            x = rng::choice<4>(rng::critical_state) + 6;
            y = rng::choice<4>(rng::critical_state) + 8;
        } while (distance(game.player().get_position(),
                          to_world_coord({x, y})) < 24);

        return to_world_coord({x, y});
    };

    auto spawn_enemy = [&] {
        auto coord = enemy_spawn_coord();
        coord.x += 16;
        switch (rng::choice<4>(rng::critical_state)) {
        case 0:
            game.enemies().spawn<Drone>(coord);
            (*game.enemies().get<Drone>().begin())->warped_in(game);
            break;

        case 1:
            if (length(game.enemies().get<Dasher>()) > 1) {
                game.enemies().spawn<Drone>(coord);
                (*game.enemies().get<Drone>().begin())->warped_in(game);
            } else {
                game.enemies().spawn<Dasher>(coord);
                (*game.enemies().get<Dasher>().begin())->warped_in(game);
            }
            break;

        case 2:
            if (length(game.enemies().get<Turret>()) > 1) {
                game.enemies().spawn<Drone>(coord);
                (*game.enemies().get<Drone>().begin())->warped_in(game);
            } else {
                game.enemies().spawn<Turret>(coord);
                (*game.enemies().get<Turret>().begin())->warped_in(game);
            }
            break;

        case 3:
            game.enemies().spawn<Scarecrow>(coord);
            (*game.enemies().get<Scarecrow>().begin())->warped_in(game);
            break;
        }
    };

    switch (state_) {
    case State::sleep:
        animate();
        spawn_timer_ += dt;
        if (spawn_timer_ > seconds(4)) {
            spawn_timer_ = 0;
            if (game.enemies().get<InfestedCore>().begin()->get() == this) {
                state_ = State::spawn_2;
            } else {
                state_ = State::await;
            }

            if (game.enemies().get<InfestedCore>().begin()->get() == this) {
                pfrm.speaker().play_music("omega", 0);
            }
        }
        break;

    case State::active: {
        animate();

        int remaining = 0;
        for (auto& core : game.enemies().get<InfestedCore>()) {
            if (core->get_sprite().get_texture_index() not_eq 48) {
                ++remaining;
            }
        }
        if (remaining == 0) {
            big_explosion(pfrm, game, position_);
            this->kill();
        }

        spawn_timer_ += dt;
        if (spawn_timer_ > milliseconds(5)) {
            spawn_timer_ = 0;

            auto& cores = game.enemies().get<InfestedCore>();
            int cores_remaining = 0;
            for (auto& _ : cores) {
                (void)_;
                cores_remaining += 1;
            }

            if (enemies_remaining(game) == cores_remaining) {
                state_ = State::vulnerable;
                top_.set_texture_index(43);
            }
        }
        break;
    }

    case State::await:
        for (auto& core : game.enemies().get<InfestedCore>()) {
            if (core->state_ == State::active) {
                state_ = State::active;
                break;
            }
        }
        break;

    case State::spawn_1:
        animate();
        spawn_timer_ += dt;
        if (spawn_timer_ > seconds(1)) {
            spawn_timer_ = 0;

            spawn_enemy();
            state_ = State::spawn_2;
        }
        break;

    case State::spawn_2:
        animate();
        spawn_timer_ += dt;
        if (spawn_timer_ > seconds(1)) {
            spawn_timer_ = 0;
            spawn_enemy();
            state_ = State::spawn_3;
        }
        break;

    case State::spawn_3:
        animate();
        spawn_timer_ += dt;
        if (spawn_timer_ > seconds(1)) {
            spawn_timer_ = 0;
            spawn_enemy();
            state_ = State::spawn_4;
        }
        break;

    case State::spawn_4:
        animate();
        spawn_timer_ += dt;
        if (spawn_timer_ > seconds(1)) {
            spawn_timer_ = 0;
            if (rng::choice<2>(rng::critical_state)) {
                auto coord = enemy_spawn_coord();
                if (game.enemies().spawn<Golem>(coord)) {
                    (*game.enemies().get<Golem>().begin())->warped_in(game);
                }
            } else {
                spawn_enemy();
            }
            state_ = State::active;
        }
        break;

    case State::vulnerable:
        animate();
        spawn_timer_ += dt;

        int remaining = 0;
        for (auto& core : game.enemies().get<InfestedCore>()) {
            if (core->get_sprite().get_texture_index() not_eq 48) {
                ++remaining;
            }
        }
        if (remaining == 0) {
            big_explosion(pfrm, game, position_);
            this->kill();
        }

        if (spawn_timer_ > seconds(12)) {
            spawn_timer_ = 0;
            // We have multiple instances of InfestedCore at a time, and we do
            // not want all of them to spawn enemies. So yeild unless we're
            // first in the list.
            int cc = 0;
            for (auto& core : game.enemies().get<InfestedCore>()) {
                if (core->get_sprite().get_texture_index() not_eq 48) {
                    ++cc;
                }
            }
            if (game.enemies().get<InfestedCore>().begin()->get() == this) {
                if (cc < 3) {
                    state_ = State::spawn_1;
                } else {
                    state_ = State::spawn_2;
                }
            } else {
                state_ = State::await;
            }
            top_.set_texture_index(50);
        }
        break;
    }

    fade_color_anim_.advance(sprite_, dt);
    top_.set_mix(sprite_.get_mix());
}


void InfestedCore::injured(Platform& pfrm, Game& game, Health amount)
{
    if (state_ not_eq State::vulnerable) {
        const auto c = current_zone(game).energy_glow_color_;
        sprite_.set_mix({c, 255});
        return;
    }

    if (sprite_.get_mix().amount_ < 180) {
        pfrm.sleep(2);
    }

    damage_ += amount;

    if (alive()) {
        pfrm.speaker().play_sound("click", 1, position_);
    }

    if (get_health() > 1) {
        debit_health(pfrm, amount);
    } else {
        if (sprite_.get_texture_index() not_eq 48) {
            game.effects().clear();
            big_explosion(pfrm, game, position_);
            pfrm.speaker().play_sound("explosion1", 3, position_);
            sprite_.set_texture_index(48);
            push_notification(
                pfrm,
                game.state(),
                locale_string(pfrm, LocaleString::boss3_core_cleansed)
                    ->c_str());
        }
        return;
    }

    auto& cores = game.enemies().get<InfestedCore>();

    Health remaining = 0;
    for (auto& core : cores) {
        remaining += core->get_health();
    }

    show_boss_health(pfrm, game, 0, Float(remaining) / (initial_health * 4));

    const auto c = current_zone(game).injury_glow_color_;
    sprite_.set_mix({c, 255});
    top_.set_mix({c, 255});
}


void InfestedCore::on_collision(Platform& pfrm, Game& game, Laser&)
{
    injured(pfrm, game, 1);
}
