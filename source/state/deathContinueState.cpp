#include "state_impl.hpp"


void DeathContinueState::enter(Platform& pfrm, Game& game, State&)
{
    game.player().set_visible(false);

    for (auto& score : reversed(game.highscores())) {
        if (score < game.score()) {
            score = game.score();
            break;
        }
    }
    std::sort(game.highscores().rbegin(), game.highscores().rend());
}


enum { max_page = 3 };


void DeathContinueState::exit(Platform& pfrm, Game& game, State&)
{
    lines_.clear();

    pfrm.fill_overlay(0);
}


void DeathContinueState::repaint_stats(Platform& pfrm, Game& game)
{
    lines_.clear();

    const auto screen_tiles = calc_screen_tiles(pfrm);

    pfrm.set_tile(
        Layer::overlay, screen_tiles.x - 2, 11, page_ == max_page ? 174 : 172);

    pfrm.set_tile(Layer::overlay, 1, 11, page_ == 0 ? 175 : 173);

    const auto dot = locale_string(pfrm, LocaleString::punctuation_period);

    auto print_metric = [&](const char* str,
                            int num,
                            const char* suffix = "",
                            bool highlight = false) {
        if (lines_.full()) {
            return;
        }

        lines_.emplace_back(pfrm, Vec2<u8>{3, u8(8 + 2 * lines_.size())});

        const auto colors =
            highlight
                ? Text::OptColors{FontColors{ColorConstant::rich_black,
                                             ColorConstant::aerospace_orange}}
                : std::nullopt;


        lines_.back().append(str, colors);

        const auto iters =
            screen_tiles.x -
            (utf8::len(str) + 6 + integer_text_length(num) + utf8::len(suffix));


        for (u32 i = 0; i < iters; ++i) {
            lines_.back().append(dot->c_str(), colors);
        }

        lines_.back().append(num, colors);
        lines_.back().append(suffix, colors);
    };

    auto print_heading = [&](const char* str) {
        if (lines_.full()) {
            return;
        }
        const auto margin = centered_text_margins(pfrm, str_len(str));
        lines_.emplace_back(pfrm,
                            Vec2<u8>{u8(margin), u8(8 + 2 * lines_.size())});
        lines_.back().assign(str);
    };

    bool highlighted = false;

    auto show_highscores = [&](int offset) {
        print_heading(locale_string(pfrm, LocaleString::high_scores)->c_str());
        for (u32 i = 0; i < lines_.capacity(); ++i) {
            char str[8];
            locale_num2str(i + 1 + offset, str, 10);
            StringBuffer<9> fmt = str;
            fmt += " ";

            const auto score = game.highscores()[i + offset];

            auto highlight = score == game.score() and not highlighted;
            if (highlight) {
                highlighted = true;
            }

            print_metric(fmt.c_str(), score, "", highlight);
        }
    };

    switch (page_) {
    case 0:
        print_heading(
            locale_string(pfrm, LocaleString::overall_heading)->c_str());
        print_metric(locale_string(pfrm, LocaleString::score)->c_str(),
                     game.score());
        print_metric(locale_string(pfrm, LocaleString::high_score)->c_str(),
                     game.highscores()[0]);
        print_metric(locale_string(pfrm, LocaleString::waypoints)->c_str(),
                     game.level());
        print_metric(
            locale_string(pfrm, LocaleString::items_collected_prefix)->c_str(),
            100 * items_collected_percentage(game.inventory()),
            locale_string(pfrm, LocaleString::items_collected_suffix)->c_str());
        break;

    case 1:
        show_highscores(0);
        break;

    case 2:
        show_highscores(4);
        break;

    case 3: {
        print_heading(locale_string(pfrm, LocaleString::items_collected_heading)
                          ->c_str());

        auto write_percentage = [&](LocaleString str, int zone) {
            StringBuffer<31> fmt = locale_string(pfrm, str)->c_str();
            fmt.pop_back();
            fmt += " ";
            print_metric(
                fmt.c_str(),
                100 * items_collected_percentage(game.inventory(), zone));
        };

        write_percentage(LocaleString::part_1_text, 0);
        write_percentage(LocaleString::part_2_text, 1);
        write_percentage(LocaleString::part_3_text, 2);
        write_percentage(LocaleString::part_4_text, 3);

    } break;
    }
}


StatePtr
DeathContinueState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    auto refresh = [&] {
        lines_.clear();
        locked_ = true;
        game.on_timeout(
            pfrm, milliseconds(100), [this](Platform& pfrm, Game& game) {
                repaint_stats(pfrm, game);
                locked_ = false;
                pfrm.speaker().play_sound("scroll", 1);
            });
    };

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

                game.powerups().clear();

                rng::get(rng::critical_state);

                repaint_stats(pfrm, game);

                game.persistent_data().seed_ = rng::critical_state;
                game.inventory().remove_non_persistent();

                PersistentData& data = game.persistent_data().reset(pfrm);
                pfrm.write_save_data(&data, sizeof data);
            }
        } else {
            if (pfrm.keyboard().down_transition<Key::right>()) {
                if (page_ < max_page) {
                    ++page_;
                    refresh();
                }
            } else if (pfrm.keyboard().down_transition<Key::left>()) {
                if (page_ > 0) {
                    --page_;
                    refresh();
                }
            }
        }

        if (not locked_ and not lines_.empty()) {
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
