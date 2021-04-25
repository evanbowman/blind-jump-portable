#include "script/lisp.hpp"
#include "state_impl.hpp"


static auto fade_in_color(const Game& game)
{
    if (game.level() == 0) {
        return custom_color(0xaec7c1);
    } else {
        return custom_color(0xeaf3cd);
    }
}


static void swap_background(Platform& pfrm, const char* image)
{
    pfrm.load_tile0_texture(image);

    const auto screen_tiles = calc_screen_tiles(pfrm);

    const bool bigfont = locale_requires_doublesize_font();

    int start_y = bigfont ? 2 : 3;

    draw_image(pfrm, 1, 0, start_y, 30, 14, Layer::background);

    for (int i = 0; i < screen_tiles.x; ++i) {
        pfrm.set_tile(Layer::background, i, start_y - 1, 9);
    }

    for (int i = 11; i < 23; ++i) {
        pfrm.set_tile(Layer::background, i, start_y, 9);
    }

    for (int x = 0; x < TileMap::width; ++x) {
        for (int y = 0; y < TileMap::height; ++y) {
            pfrm.set_tile(Layer::map_0, x, y, 1);
            pfrm.set_tile(Layer::map_1, x, y, 0);
        }
    }
}


void TitleScreenState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.screen().fade(1.f);

    pfrm.set_overlay_origin(0, 0);
    game.camera() = {};

    pfrm.speaker().play_music("midsommar", 0);
    pfrm.speaker().stop_music();

    pfrm.load_overlay_texture("overlay");
    pfrm.load_tile1_texture("tilesheet_top");

    const Vec2<Float> arbitrary_offscreen_location{1000, 1000};

    game.transporter().set_position(arbitrary_offscreen_location);
    game.player().set_visible(false);

    pfrm.enable_glyph_mode(true);

    const auto screen_tiles = calc_screen_tiles(pfrm);

    const bool bigfont = locale_requires_doublesize_font();

    for (int i = 0; i < screen_tiles.x; ++i) {
        pfrm.set_tile(Layer::overlay, i, 0, 112);

        if (not bigfont) {
            pfrm.set_tile(Layer::overlay, i, 1, 112);
        }

        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 1, 112);
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 2, 112);
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 3, 112);

        if (bigfont) {
            pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 4, 112);
        }
    }

    pfrm.screen().fade(1.f, fade_in_color(game));

    if (game.level() == 0) {
        swap_background(pfrm, "title_1_flattened");
    } else {
        swap_background(pfrm, "title_2_flattened");
    }
}


void TitleScreenState::exit(Platform& pfrm, Game& game, State& next_state)
{
    pfrm.fill_overlay(0);
    title_.reset();
}


static void
draw_title(Platform& pfrm, Game& game, int sel, std::optional<Text>& title)
{
    auto str = locale_string(pfrm, LocaleString::game_title);

    const bool bigfont = locale_requires_doublesize_font();

    auto margin = centered_text_margins(pfrm, utf8::len(str->c_str()));

    if (bigfont) {
        margin -= utf8::len(str->c_str()) / 2;
    }

    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;

    const u8 title_y = bigfont ? 2 : 3;

    title.emplace(pfrm, OverlayCoord{u8(margin), title_y}, font_conf);

    const auto text_bg_color = [&] {
        if (game.level() == 0 or sel == 1) {
            return custom_color(0x4091AD);
        } else {
            return custom_color(0x9c9599);
        }
    }();

    const auto text_fg_color = [&] {
        if (game.level() == 0 or sel == 1) {
            return custom_color(0xFFFFFF);
        } else {
            return custom_color(0x134675);
        }
    }();

    title->assign(str->c_str(),
                  Text::OptColors{{text_fg_color, text_bg_color}});
}


StatePtr
TitleScreenState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    auto animate_selector = [&] {
        if (timer_ > milliseconds(75)) {
            timer_ = 0;
            const auto st = calc_screen_tiles(pfrm);
            auto& text = *options_[cursor_index_];
            auto get_coords = [&](Text& text) -> Vec2<u8> {
                const u8 left_x = text.coord().x - 2;
                const bool bigfont = locale_requires_doublesize_font();
                const auto width = bigfont ? text.len() * 2 : text.len();
                const u8 right_x = text.coord().x + width + 1;
                return {left_x, right_x};
            };
            const auto c1 = get_coords(text);
            if (pfrm.get_tile(Layer::overlay, c1.x, st.y - 2) == 375) {
                pfrm.set_tile(Layer::overlay, c1.x, st.y - 2, 373);
                pfrm.set_tile(Layer::overlay, c1.y, st.y - 2, 374);
            } else {
                pfrm.set_tile(Layer::overlay, c1.x, st.y - 2, 375);
                pfrm.set_tile(Layer::overlay, c1.y, st.y - 2, 376);
            }

            auto erase_coords = get_coords(*options_[!cursor_index_]);
            pfrm.set_tile(Layer::overlay, erase_coords.x, st.y - 2, 112);
            pfrm.set_tile(Layer::overlay, erase_coords.y, st.y - 2, 112);
        }
    };

    auto prep_exit = [&] {
        display_mode_ = DisplayMode::fade_out;
        timer_ = 0;
        title_.reset();
        options_[0].reset();
        options_[1].reset();

        const auto screen_tiles = calc_screen_tiles(pfrm);

        const bool bigfont = locale_requires_doublesize_font();

        for (int i = 0; i < screen_tiles.x; ++i) {
            pfrm.set_tile(Layer::overlay, i, 0, 112);
            pfrm.set_tile(Layer::overlay, i, 1, 112);

            pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 1, 112);
            pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 2, 112);
            pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 3, 112);

            if (bigfont) {
                pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 4, 112);
            }
        }
    };

    switch (display_mode_) {
    case DisplayMode::sleep:
        timer_ += delta;
        if (timer_ > seconds(1)) {
            timer_ = 0;
            display_mode_ = DisplayMode::fade_in;
        }
        break;

    case DisplayMode::select:
        timer_ += delta;
        animate_selector();
        if (pfrm.keyboard().down_transition(game.action2_key())) {
            pfrm.speaker().play_sound("select", 1);
            if (cursor_index_ == 0) {
                if (game.persistent_data().clean_) {
                    break;
                } else {
                    // We just resumed a save file. Now, write an empty save
                    // back, so that a player cannot simply cut the power when
                    // they die and resume from the same save file again.
                    auto data = game.persistent_data();
                    data.reset(pfrm);
                    pfrm.write_save_data(&data, sizeof(data), 0);
                }
            } else {
                newgame(pfrm, game);
            }
            prep_exit();

        } else if (pfrm.keyboard().down_transition<Key::left>()) {
            if (cursor_index_ > 0) {
                --cursor_index_;
                timer_ = seconds(1);
                animate_selector();
                timer_ = 0;
                display_mode_ = DisplayMode::image_animate_out;
                pfrm.speaker().play_sound("scroll", 1);

                const bool bigfont = locale_requires_doublesize_font();
                const auto st = calc_screen_tiles(pfrm);
                const u8 start_y = 2 - (bigfont ? 1 : 0);

                sidebar_.emplace(pfrm,
                                 u8(st.x),
                                 u8(st.y - 5),
                                 OverlayCoord{u8(st.x), start_y});
            }
        } else if (pfrm.keyboard().down_transition<Key::right>()) {
            if (cursor_index_ < 1) {
                ++cursor_index_;
                timer_ = seconds(1);
                animate_selector();
                timer_ = 0;
                display_mode_ = DisplayMode::image_animate_out;
                pfrm.speaker().play_sound("scroll", 1);

                const bool bigfont = locale_requires_doublesize_font();
                const auto st = calc_screen_tiles(pfrm);
                const u8 start_y = 2 - (bigfont ? 1 : 0);

                sidebar2_.emplace(
                    pfrm, u8(st.x), u8(st.y - 5), OverlayCoord{0, start_y});
            }
        } else if (pfrm.keyboard().down_transition<Key::select>()) {
            if (++bossrush_cheat_counter_ == 15) {
                newgame(pfrm, game);
                game.player().set_health(5);
                lisp::set_var("debug-mode", lisp::make_integer(7));
                prep_exit();
            }
        }
        break;

    case DisplayMode::image_animate_out: {
        timer_ += delta;
        constexpr auto anim_duration = milliseconds(200);
        if (timer_ > anim_duration) {
            timer_ = seconds(1);
            animate_selector();
            timer_ = 0;
            pfrm.screen().fade(1.f);
            sidebar_.reset();
            sidebar2_.reset();
            display_mode_ = DisplayMode::swap_image;
        } else {
            const auto amount = smoothstep(0.f, anim_duration, timer_);
            if (sidebar_) {
                sidebar_->set_display_percentage(amount);
            }
            if (sidebar2_) {
                sidebar2_->set_display_percentage(amount);
            }
        }
    } break;

    case DisplayMode::swap_image:
        display_mode_ = DisplayMode::image_animate_in;
        switch (cursor_index_) {
        case 0:
            if (game.level() not_eq 0) {
                swap_background(pfrm, "title_2_flattened");
            }
            break;

        case 1:
            swap_background(pfrm, "title_1_flattened");
            break;
        }
        break;

    case DisplayMode::image_animate_in: {
        timer_ += delta;
        constexpr auto fade_duration = milliseconds(100);
        if (timer_ > fade_duration) {
            timer_ = seconds(1);
            pfrm.screen().fade(0.f);
            draw_title(pfrm, game, cursor_index_, title_);
            display_mode_ = DisplayMode::select;
        } else {
            pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, timer_),
                               ColorConstant::rich_black);
        }
    } break;

    case DisplayMode::fade_in: {
        timer_ += delta;

        constexpr auto fade_duration = milliseconds(1500);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(0.f);

            timer_ = 0;
            display_mode_ = DisplayMode::wait;

        } else {
            pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, timer_),
                               fade_in_color(game));
        }
    } break;

    case DisplayMode::wait:
        timer_ += delta;
        if (timer_ > milliseconds(300)) {

            draw_title(pfrm, game, cursor_index_, title_);

            auto opt_1 = locale_string(pfrm, LocaleString::continue_str);
            auto opt_2 = locale_string(pfrm, LocaleString::newgame_str);

            const bool bigfont = locale_requires_doublesize_font();


            const auto len_1 = utf8::len(opt_1->c_str()) * (bigfont ? 2 : 1);
            const auto len_2 = utf8::len(opt_2->c_str()) * (bigfont ? 2 : 1);

            const auto st = calc_screen_tiles(pfrm);

            const auto spacing = (st.x - (len_1 + len_2)) / 3;

            const u8 start_y = bigfont ? st.y - 3 : st.y - 2;

            FontConfiguration font_conf;
            font_conf.double_size_ = bigfont;

            options_[0].emplace(
                pfrm, OverlayCoord{u8(spacing), start_y}, font_conf);

            options_[0]->assign(opt_1->c_str(), [&]() -> Text::OptColors {
                if (not game.persistent_data().clean_) {
                    return std::nullopt;
                } else {
                    return FontColors{ColorConstant::med_blue_gray,
                                      ColorConstant::rich_black};
                }
            }());

            if (game.persistent_data().clean_) {
                cursor_index_ = 1;
            }

            u8 bugfix_for_long_russian_text = 0;
            if (len_1 + len_2 == 20) {
                // The newgame and continue text in russian take up all
                // available characters, we need to throw in this little bugfix
                // to adjust the balance of the text, so that the arrow icons
                // that wrap the text do not overlap and create issues.
                bugfix_for_long_russian_text = 1;
            }

            options_[1].emplace(pfrm,
                                opt_2->c_str(),
                                OverlayCoord{u8(spacing * 2 + len_1 +
                                                bugfix_for_long_russian_text),
                                             start_y},
                                font_conf);


            display_mode_ = DisplayMode::select;
        }
        break;

    case DisplayMode::fade_out: {
        timer_ += delta;

        constexpr auto fade_duration = milliseconds(670);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(1.f);

            timer_ = 0;
            display_mode_ = DisplayMode::pause;

        } else {
            pfrm.screen().fade(smoothstep(0.f, fade_duration, timer_),
                               ColorConstant::rich_black,
                               {},
                               true,
                               true);

            return null_state();
        }
    } break;

    case DisplayMode::pause:
        timer_ += delta;
        if (timer_ > seconds(1)) {
            timer_ = 0;
            if (game.level() == 0) {
                auto str = locale_string(pfrm, LocaleString::intro_text_2);
                return state_pool().create<IntroCreditsState>(std::move(str));
            } else {
                return state_pool().create<NewLevelState>(game.level());
            }
        }
        break;
    }

    return null_state();
}
