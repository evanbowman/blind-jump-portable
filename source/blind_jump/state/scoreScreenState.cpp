#include "state_impl.hpp"


enum { max_page = 3 };


void ScoreScreenState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    playtime_seconds_ = game.persistent_data().speedrun_clock_.whole_seconds();
    max_level_ = game.level();

    game.player().set_visible(false);

    for (auto& score : reversed(game.highscores())) {
        if (score.get() < game.score()) {
            score.set(game.score());
            break;
        }
    }
    std::sort(std::begin(game.highscores()),
              std::end(game.highscores()),
              [](auto& lhs, auto& rhs) { return lhs.get() > rhs.get(); });

    game.powerups().clear();

    rng::get(rng::critical_state);

    game.persistent_data().seed_.set(rng::critical_state);
    game.inventory().remove_non_persistent();

    PersistentData& data = game.persistent_data().reset(pfrm);
    data.clean_ = false;
    pfrm.write_save_data(&data, sizeof data, 0);
}


void ScoreScreenState::exit(Platform& pfrm, Game& game, State& next_state)
{
    clear_stats(pfrm);
}


void ScoreScreenState::clear_stats(Platform& pfrm)
{
    lines_.clear();

    const auto screen_tiles = calc_screen_tiles(pfrm);

    pfrm.set_tile(Layer::overlay, screen_tiles.x - 2, 11, 0);

    pfrm.set_tile(Layer::overlay, 1, 11, 0);
}


StatePtr
ScoreScreenState::update(Platform& pfrm, Game& game, Microseconds delta)
{
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

    return null_state();
}


void ScoreScreenState::repaint_stats(Platform& pfrm, Game& game)
{
    lines_.clear();

    const auto screen_tiles = calc_screen_tiles(pfrm);

    pfrm.set_tile(
        Layer::overlay, screen_tiles.x - 2, 11, page_ == max_page ? 174 : 172);

    pfrm.set_tile(Layer::overlay, 1, 11, page_ == 0 ? 175 : 173);

    const auto dot = locale_string(pfrm, LocaleString::punctuation_period);

    auto print_metric_impl = [&](const char* str,
                                 const StringBuffer<32>& text,
                                 const char* suffix = "",
                                 bool highlight = false) {
        if (lines_.full()) {
            return;
        }

        lines_.emplace_back(
            pfrm, Vec2<u8>{3, u8(metrics_y_offset_ + 8 + 2 * lines_.size())});

        const auto colors =
            highlight
                ? Text::OptColors{FontColors{ColorConstant::rich_black,
                                             ColorConstant::aerospace_orange}}
                : metric_font_colors_;


        lines_.back().append(str, colors);

        const auto iters = screen_tiles.x - (utf8::len(str) + 6 +
                                             text.length() + utf8::len(suffix));


        for (u32 i = 0; i < iters; ++i) {
            lines_.back().append(dot->c_str(), colors);
        }

        lines_.back().append(text.c_str(), colors);
        lines_.back().append(suffix, colors);
    };


    auto print_metric = [&](const char* str,
                            int num,
                            const char* suffix = "",
                            bool highlight = false) {
        char buffer[32];
        locale_num2str(num, buffer, 10);
        print_metric_impl(str, buffer, suffix, highlight);
    };

    auto print_heading = [&](const char* str) {
        if (lines_.full()) {
            return;
        }

        const bool bigfont = locale_requires_doublesize_font();
        FontConfiguration font_conf;
        font_conf.double_size_ = bigfont;


        const auto margin =
            centered_text_margins(pfrm, utf8::len(str) * (bigfont ? 2 : 1));
        lines_.emplace_back(pfrm,
                            Vec2<u8>{u8(margin),
                                     u8(metrics_y_offset_ + 8 +
                                        2 * lines_.size() - (bigfont ? 1 : 0))},
                            font_conf);

        lines_.back().assign(str, metric_font_colors_);
    };

    bool highlighted = false;

    auto show_highscores = [&](int offset) {
        print_heading(locale_string(pfrm, LocaleString::high_scores)->c_str());
        for (u32 i = 0; i < lines_.capacity(); ++i) {
            char str[8];
            locale_num2str(i + 1 + offset, str, 10);
            StringBuffer<9> fmt = str;
            fmt += " ";

            const auto score = game.highscores()[i + offset].get();

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
        print_metric(locale_string(pfrm, LocaleString::waypoints)->c_str(),
                     max_level_);
        print_metric(
            locale_string(pfrm, LocaleString::items_collected_prefix)->c_str(),
            100 * items_collected_percentage(game.inventory()),
            locale_string(pfrm, LocaleString::items_collected_suffix)->c_str());
        print_metric_impl(locale_string(pfrm, LocaleString::time)->c_str(),
                          format_time(playtime_seconds_));
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

            if (not(locale_language_name(locale_get_language()) == "chinese")) {
                // No colon after the zone title in chinese text

                fmt.pop_back();
            }

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
