#include "state_impl.hpp"


void DeathContinueState::enter(Platform& pfrm, Game& game, State&)
{
    game.player().set_visible(false);
}


void DeathContinueState::exit(Platform& pfrm, Game& game, State&)
{
    score_.reset();
    highscore_.reset();
    level_.reset();
    items_collected_.reset();

    pfrm.fill_overlay(0);
}


StatePtr
DeathContinueState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    constexpr auto fade_duration = seconds(1);
    if (counter_ > fade_duration) {
        counter_ -= delta;
        pfrm.screen().fade(1.f,
                           ColorConstant::rich_black,
                           current_zone(game).injury_glow_color_);

        constexpr auto stats_time = milliseconds(500);

        if (counter2_ < stats_time) {
            const bool show_stats = counter2_ + delta > stats_time;

            counter2_ += delta;

            if (show_stats) {
                score_.emplace(pfrm, Vec2<u8>{1, 8});
                highscore_.emplace(pfrm, Vec2<u8>{1, 10});
                level_.emplace(pfrm, Vec2<u8>{1, 12});
                items_collected_.emplace(pfrm, Vec2<u8>{1, 14});

                const auto screen_tiles = calc_screen_tiles(pfrm);

                auto print_metric = [&](Text& target,
                                        const char* str,
                                        int num,
                                        const char* suffix = "") {
                    target.append(str);

                    const auto iters =
                        screen_tiles.x -
                        (utf8::len(str) + 2 + integer_text_length(num) +
                         utf8::len(suffix));
                    for (u32 i = 0; i < iters; ++i) {
                        target.append(
                            locale_string(LocaleString::punctuation_period));
                    }

                    target.append(num);
                    target.append(suffix);
                };

                game.powerups().clear();

                for (auto& score : reversed(game.highscores())) {
                    if (score < game.score()) {
                        score = game.score();
                        break;
                    }
                }
                std::sort(game.highscores().rbegin(), game.highscores().rend());

                rng::get(rng::critical_state);

                print_metric(
                    *score_, locale_string(LocaleString::score), game.score());
                print_metric(*highscore_,
                             locale_string(LocaleString::high_score),
                             game.highscores()[0]);
                print_metric(*level_,
                             locale_string(LocaleString::waypoints),
                             game.level());
                print_metric(
                    *items_collected_,
                    locale_string(LocaleString::items_collected_prefix),
                    100 * items_collected_percentage(game.inventory()),
                    locale_string(LocaleString::items_collected_suffix));

                game.persistent_data().seed_ = rng::critical_state;
                game.inventory().remove_non_persistent();

                PersistentData& data = game.persistent_data().reset(pfrm);
                pfrm.write_save_data(&data, sizeof data);
            }
        }

        if (score_) {
            if (pfrm.keyboard().pressed(game.action1_key()) or
                pfrm.keyboard().pressed(game.action2_key())) {

                game.score() = 0;
                game.player().revive(pfrm);

                return state_pool().create<RespawnWaitState>();
            }
        }
        return null_state();

    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           current_zone(game).injury_glow_color_);
        return null_state();
    }
}
