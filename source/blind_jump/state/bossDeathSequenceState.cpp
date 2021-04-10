#include "state_impl.hpp"


void BossDeathSequenceState::enter(Platform& pfrm,
                                   Game& game,
                                   State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);
    repaint_powerups(pfrm,
                     game,
                     true,
                     &health_,
                     &score_,
                     nullptr,
                     &powerups_,
                     UIMetric::Align::left);

    game.enemies().clear();
}


void BossDeathSequenceState::exit(Platform& pfrm, Game& game, State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);
    powerups_.clear();
    health_.reset();
    score_.reset();
}


StatePtr
BossDeathSequenceState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    const auto last_health = game.player().get_health();
    const auto last_score = game.score();

    game.player().update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    update_ui_metrics(pfrm,
                      game,
                      delta,
                      &health_,
                      &score_,
                      nullptr,
                      &powerups_,
                      last_health,
                      last_score,
                      false,
                      UIMetric::Align::left);

    counter_ += delta;

    auto bosses_remaining = [&] {
        for (Level l = game.level() + 1; l < game.level() + 1000; ++l) {
            if (is_boss_level(l)) {
                return true;
            }
        }
        return false;
    };

    switch (anim_state_) {
    case AnimState::init: {
        pfrm.speaker().stop_music();

        pfrm.speaker().play_sound("explosion1", 3, boss_position_);
        big_explosion(pfrm, game, boss_position_);

        const auto off = 50.f;

        big_explosion(
            pfrm, game, {boss_position_.x - off, boss_position_.y - off});
        big_explosion(
            pfrm, game, {boss_position_.x + off, boss_position_.y + off});
        counter_ = 0;
        anim_state_ = AnimState::explosion_wait1;
        break;
    }

    case AnimState::explosion_wait1:
        if (counter_ > milliseconds(300)) {
            pfrm.speaker().play_sound("explosion1", 3, boss_position_);
            big_explosion(pfrm, game, boss_position_);
            const auto off = -50.f;

            big_explosion(
                pfrm, game, {boss_position_.x - off, boss_position_.y + off});
            big_explosion(
                pfrm, game, {boss_position_.x + off, boss_position_.y - off});

            anim_state_ = AnimState::explosion_wait2;

            counter_ = 0;
        }
        break;

    case AnimState::explosion_wait2:
        if (counter_ > milliseconds(120)) {
            counter_ = 0;
            anim_state_ = AnimState::fade;

            pfrm.speaker().play_sound("explosion2", 4, boss_position_);

            for (int i = 0; i <
                            [&] {
                                switch (game.difficulty()) {
                                case Settings::Difficulty::easy:
                                    return 4;

                                case Settings::Difficulty::count:
                                case Settings::Difficulty::normal:
                                    break;

                                case Settings::Difficulty::survival:
                                case Settings::Difficulty::hard:
                                    return 2;
                                }
                                return 3;
                            }();
                 ++i) {
                game.details().spawn<Item>(
                    rng::sample<32>(boss_position_, rng::utility_state),
                    pfrm,
                    Item::Type::heart);
            }

            if (bosses_remaining()) {
                auto tc = to_tile_coord(boss_position_.cast<s32>());

                if (not is_walkable(game.tiles().get_tile(tc.x, tc.y))) {
                    for (int i = tc.x - 1; i < tc.x + 1; ++i) {
                        for (int j = tc.y - 1; j < tc.y + 1; ++j) {
                            if (is_walkable(game.tiles().get_tile(i, j))) {
                                tc.x = i;
                                tc.y = j;
                            }
                        }
                    }
                }

                auto wc = to_world_coord(tc);
                wc.x += 16;
                wc.y += 16;
                game.transporter().set_position(wc);
            } else {
                create_item_chest(game,
                                  to_world_coord(Vec2<TIdx>{8, 10}),
                                  Item::Type::long_jump_home,
                                  false);
            }
        }
        break;

    case AnimState::fade: {
        constexpr auto fade_duration = seconds(4) + milliseconds(500);
        if (counter_ > seconds(1) + milliseconds(700) and
            not pushed_notification_) {
            pushed_notification_ = true;
            push_notification(
                pfrm, this, locale_string(pfrm, defeated_text_)->c_str());
        }
        if (counter_ > fade_duration) {
            pfrm.screen().fade(0.f);
            return state_pool().create<ActiveState>();

        } else {
            const auto amount = 1.f - smoothstep(0.f, fade_duration, counter_);
            pfrm.screen().fade(amount, ColorConstant::silver_white);
        }
        break;
    }

    case AnimState::endgame: {
        constexpr auto fade_duration = seconds(8) + milliseconds(500);
        if (counter_ > fade_duration) {
            pfrm.screen().fade(1.f);
            return state_pool().create<EndingCreditsState>();
        } else {
            const auto amount = smoothstep(0.f, fade_duration, counter_);
            pfrm.screen().fade(amount);
        }
        break;
    }
    }


    return null_state();
}
