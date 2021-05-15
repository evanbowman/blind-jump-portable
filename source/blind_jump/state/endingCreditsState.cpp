#include "state_impl.hpp"


void EndingCreditsState::enter(Platform& pfrm, Game& game, State&)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    pfrm.load_tile0_texture("tilesheet3");
    game.transporter().set_position({10000, 10000});
    game.enemies().clear();
    game.details().clear();
    game.effects().clear();

    next_y_ = screen_tiles.y + 2;

    game.on_timeout(pfrm, milliseconds(500), [](Platform& pfrm, Game&) {
        pfrm.speaker().play_music("clair_de_lune", 0);
    });
}


void EndingCreditsState::exit(Platform& pfrm, Game& game, State&)
{
    lines_.clear();
    pfrm.set_overlay_origin(0, 0);
    pfrm.speaker().stop_music();

    pfrm.screen().fade(1.f, ColorConstant::rich_black, {}, false, false);
    pfrm.fill_overlay(0);
}


// NOTE: '%' characters in the credits will be filled with N '.' characters,
// such that the surrounding text is aligned to either edge of the screen.
//
// FIXME: localize the credits? Nah...
static const std::array<const char*, 33> credits_lines = {
    "Artwork and Source Code by",
    "Evan Bowman",
    "",
    "",
    "Story",
    "Evan Bowman",
    "",
    "",
    "Music",
    "Midsommar%Scott Buckley",
    "Omega%Scott Buckley",
    "Computations%Scott Buckley",
    "Murmuration%Kai Engel",
    "Chantiers Navals%LJ Kruzer",
    "Clair De Lune%Chad Crouch",
    "",
    "",
    "Playtesting",
    "Benjamin Casler",
    "",
    "",
    "Translations",
    "Chinese%verkkarscd",
    "",
    "Russian%Павел Полстюк",
    "",
    "",
    "Special Thanks",
    "Jasper Vijn (Tonc)",
    "The DevkitARM Project",
    "freesound.org"
    "",
    ""};


void draw_starfield(Platform& pfrm);


StatePtr
EndingCreditsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    timer_ += delta;

    animate_starfield(pfrm, delta);

    switch (display_mode_) {
    case DisplayMode::scroll:
        if (timer_ > milliseconds([&] {
                if (pfrm.keyboard().pressed(game.action2_key())) {
                    return 15;
                } else {
                    return 60;
                }
            }())) {

            timer_ = 0;

            pfrm.set_overlay_origin(0, scroll_++);

            constexpr int tile_height = 8;
            const bool scrolled_two_lines = scroll_ % (tile_height * 2) == 0;

            if (scrolled_two_lines) {

                auto screen_tiles = calc_screen_tiles(pfrm);

                if (scroll_ > ((screen_tiles.y + 2) * tile_height) and
                    not lines_.empty()) {
                    lines_.erase(lines_.begin());
                }

                if (next_ < int{credits_lines.size() - 1}) {
                    const u8 y =
                        next_y_ % 32; // The overlay tile layer is 32x32 tiles.
                    next_y_ += 2;

                    const auto str = credits_lines[next_++];


                    bool contains_fill_char = false;
                    utf8::scan(
                        [&contains_fill_char,
                         &pfrm](const utf8::Codepoint& cp, const char*, int) {
                            if (cp == '%') {
                                if (contains_fill_char) {
                                    pfrm.fatal("two fill chars not allowed");
                                }
                                contains_fill_char = true;
                            }
                        },
                        str,
                        str_len(str));

                    if (utf8::len(str) > u32(screen_tiles.x - 2)) {
                        pfrm.fatal("credits text too large");
                    }

                    if (contains_fill_char) {
                        const auto fill =
                            screen_tiles.x - ((utf8::len(str) - 1) + 2);
                        lines_.emplace_back(pfrm, OverlayCoord{1, y});
                        utf8::scan(
                            [this, fill](
                                const utf8::Codepoint&, const char* raw, int) {
                                if (str_len(raw) == 1 and raw[0] == '%') {
                                    for (size_t i = 0; i < fill; ++i) {
                                        lines_.back().append(".");
                                    }
                                } else {
                                    lines_.back().append(raw);
                                }
                            },
                            str,
                            str_len(str));
                    } else {
                        const auto len = utf8::len(str);
                        const u8 left_margin = (screen_tiles.x - len) / 2;
                        lines_.emplace_back(pfrm, OverlayCoord{left_margin, y});
                        lines_.back().assign(str);
                    }


                } else // if ((int)(lines_.size() + 1) == screen_tiles.y / 4 + 1)
                {
                    if (scroll_ > int(pfrm.screen().size().y +
                                      credits_lines.size() * 16)) {
                        display_mode_ = DisplayMode::scroll2;
                        scroll_ = 0;
                        timer_ = 0;
                        lines_.clear();
                        pfrm.set_overlay_origin(0, 1);

                        auto str =
                            locale_string(pfrm, LocaleString::the_end_str);

                        const auto len = utf8::len(str->c_str());

                        const bool bigfont = locale_requires_doublesize_font();

                        FontConfiguration font_conf;
                        font_conf.double_size_ = bigfont;

                        if (bigfont) {
                            const u8 left_margin =
                                (screen_tiles.x - len * 2) / 2;
                            lines_.emplace_back(
                                pfrm,
                                OverlayCoord{left_margin,
                                             u8(screen_tiles.y + 1)},
                                font_conf);
                        } else {
                            const u8 left_margin = (screen_tiles.x - len) / 2;
                            lines_.emplace_back(
                                pfrm,
                                OverlayCoord{left_margin,
                                             u8(screen_tiles.y + 1)},
                                font_conf);
                        }

                        lines_.back().assign(str->c_str());
                    }
                }
            }
        }
        break;

    case DisplayMode::scroll2:
        if (timer_ > milliseconds([&] {
                if (pfrm.keyboard().pressed(game.action2_key())) {
                    return 15;
                } else {
                    return 60;
                }
            }())) {

            timer_ = 0;

            pfrm.set_overlay_origin(0, scroll_++);

            if (scroll_ == 90) {
                display_mode_ = DisplayMode::draw_image;
                auto screen_tiles = calc_screen_tiles(pfrm);

                auto str = locale_string(pfrm, LocaleString::the_end_str);

                const auto len = utf8::len(str->c_str());

                const bool bigfont = locale_requires_doublesize_font();

                FontConfiguration font_conf;
                font_conf.double_size_ = bigfont;

                if (bigfont) {

                    const u8 left_margin = (screen_tiles.x - len * 2) / 2;
                    for (u32 i = 0; i < len * 2; ++i) {
                        pfrm.set_tile(Layer::overlay, left_margin + i, 9, 425);
                    }
                    lines_.emplace_back(
                        pfrm, OverlayCoord{left_margin, 10}, font_conf);

                } else {

                    const u8 left_margin = (screen_tiles.x - len) / 2;
                    for (u32 i = 0; i < len; ++i) {
                        pfrm.set_tile(Layer::overlay, left_margin + i, 9, 425);
                    }
                    lines_.emplace_back(
                        pfrm, OverlayCoord{left_margin, 10}, font_conf);
                }

                lines_.back().assign(str->c_str());
            }
        }
        break;

    case DisplayMode::draw_image:
        display_mode_ = DisplayMode::fade_show_image;
        timer_ = 0;
        pfrm.set_overlay_origin(0, 0);
        game.camera().set_position(pfrm, {0, 0});

        for (int x = 0; x < 16; ++x) {
            for (int y = 0; y < 20; ++y) {
                pfrm.set_tile(Layer::map_0, x, y, 0);
                pfrm.set_tile(Layer::map_1, x, y, 0);
            }
        }

        for (int x = 0; x < 31; ++x) {
            for (int y = 0; y < 31; ++y) {
                pfrm.set_tile(Layer::background, x, y, 60);
                if (not(x > 3 and x < 26 and y > 3 and y < 17)) {
                    pfrm.set_tile(Layer::overlay, x, y, 112);
                }
            }
        }

        // So that the starfield looks the same every time. We're in the ending
        // credits, so we're about to perform a factory reset anyway.
        rng::critical_state = 10;

        draw_starfield(pfrm);

        draw_image(pfrm, 120, 10, 6, 9, 9, Layer::background);
        break;

    case DisplayMode::fade_show_image: {
        constexpr auto fade_duration = milliseconds(800);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(0.f);
            display_mode_ = DisplayMode::show_image;
            timer_ = 0;
        } else {
            const auto amount = 1.f - smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(amount);
        }
        break;
    }

    case DisplayMode::show_image:
        if (timer_ > seconds(4)) {
            timer_ = 0;
            display_mode_ = DisplayMode::fade_out;
        }
        break;

    case DisplayMode::fade_out: {
        constexpr auto fade_duration = seconds(2) + milliseconds(670);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(1.f, ColorConstant::rich_black, {}, true, true);

            display_mode_ = DisplayMode::done;
            timer_ = 0;
        } else {
            pfrm.screen().fade(smoothstep(0.f, fade_duration, timer_),
                               ColorConstant::rich_black,
                               {},
                               true,
                               true);

            return null_state();
        }

        break;
    }

    case DisplayMode::done:
        if (timer_ > seconds(2)) {
            return state_pool().create<TitleScreenState>();
        }
        break;
    }

    return null_state();
}
