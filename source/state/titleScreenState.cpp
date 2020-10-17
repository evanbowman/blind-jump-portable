#include "state_impl.hpp"


static const auto fade_in_color = custom_color(0xaec7c1);


void TitleScreenState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.screen().fade(1.f);

    for (int x = 0; x < TileMap::width; ++x) {
        for (int y = 0; y < TileMap::height; ++y) {
            pfrm.set_tile(Layer::map_0, x, y, 1);
            pfrm.set_tile(Layer::map_1, x, y, 0);
        }
    }

    pfrm.set_overlay_origin(0, 0);
    game.camera() = {};

    pfrm.speaker().play_music("midsommar", 0);
    pfrm.speaker().stop_music();

    pfrm.load_overlay_texture("overlay");
    pfrm.load_tile0_texture("title_1_flattened");
    pfrm.load_tile1_texture("tilesheet_top");

    const Vec2<Float> arbitrary_offscreen_location{1000, 1000};

    game.transporter().set_position(arbitrary_offscreen_location);
    game.player().set_visible(false);

    pfrm.enable_glyph_mode(true);

    const auto screen_tiles = calc_screen_tiles(pfrm);

    for (int i = 0; i < screen_tiles.x; ++i) {
        pfrm.set_tile(Layer::overlay, i, 0, 112);
        pfrm.set_tile(Layer::overlay, i, 1, 112);

        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 1, 112);
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 2, 112);
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 3, 112);
    }

    pfrm.screen().fade(1.f, fade_in_color);

    draw_image(pfrm, 1, 0, 3, 30, 14, Layer::background);

    for (int i = 0; i < screen_tiles.x; ++i) {
        pfrm.set_tile(Layer::background, i, 2, 8);
    }

    // We need to share vram memory for the background image and the tileset
    // layer. Therefore, in order for the tilemap not to show up, we need to
    // designate part of the image as one transparent 4x3 chunk. Now, this code
    // covers up that transparent chunk with another tile.
    for (int i = 11; i < 23; ++i) {
        pfrm.set_tile(Layer::background, i, 3, 8);
    }
}


void TitleScreenState::exit(Platform& pfrm, Game& game, State& next_state)
{
    pfrm.fill_overlay(0);
    title_.reset();
}


StatePtr TitleScreenState::update(Platform& pfrm,
                                  Game& game,
                                  Microseconds delta)
{
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
        if (timer_ > milliseconds(75)) {
            timer_ = 0;
            const auto st = calc_screen_tiles(pfrm);
            auto& text = *options_[cursor_index_];
            auto get_coords = [&](Text& text) -> Vec2<u8> {
                const u8 left_x = text.coord().x - 2;
                const u8 right_x = text.coord().x + text.len() + 1;
                return {left_x, right_x};
            };
            const auto c1 = get_coords(text);
            if (pfrm.get_tile(Layer::overlay, c1.x, st.y - 2) == 149) {
                pfrm.set_tile(Layer::overlay, c1.x, st.y - 2, 147);
                pfrm.set_tile(Layer::overlay, c1.y, st.y - 2, 148);
            } else {
                pfrm.set_tile(Layer::overlay, c1.x, st.y - 2, 149);
                pfrm.set_tile(Layer::overlay, c1.y, st.y - 2, 150);
            }

            auto erase_coords = get_coords(*options_[!cursor_index_]);
            pfrm.set_tile(Layer::overlay, erase_coords.x, st.y - 2, 112);
            pfrm.set_tile(Layer::overlay, erase_coords.y, st.y - 2, 112);
        }
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
                    pfrm.write_save_data(&data, sizeof(data));
                }
            } else {
                newgame(pfrm, game);
            }
            display_mode_ = DisplayMode::fade_out;
            timer_ = 0;
            title_.reset();
            options_[0].reset();
            options_[1].reset();

            const auto screen_tiles = calc_screen_tiles(pfrm);

            for (int i = 0; i < screen_tiles.x; ++i) {
                pfrm.set_tile(Layer::overlay, i, 0, 112);
                pfrm.set_tile(Layer::overlay, i, 1, 112);

                pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 1, 112);
                pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 2, 112);
                pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 3, 112);
            }

        } else if (pfrm.keyboard().down_transition<Key::left>()) {
            if (cursor_index_ > 0) {
                --cursor_index_;
                pfrm.speaker().play_sound("scroll", 1);
                timer_ = seconds(1);
            }
        } else if (pfrm.keyboard().down_transition<Key::right>()) {
            if (cursor_index_ < 1) {
                ++cursor_index_;
                pfrm.speaker().play_sound("scroll", 1);
                timer_ = seconds(1);
            }
        }
        break;

    case DisplayMode::fade_in: {
        timer_ += delta;

        constexpr auto fade_duration = milliseconds(1500);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(0.f);

            timer_ = 0;
            display_mode_ = DisplayMode::wait;

        } else {
            pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, timer_),
                               fade_in_color);
        }
    } break;

    case DisplayMode::wait:
        timer_ += delta;
        if (timer_ > milliseconds(300)) {
            auto str = locale_string(pfrm, LocaleString::game_title);
            const auto margin =
                centered_text_margins(pfrm, utf8::len(str->c_str()));

            title_.emplace(pfrm, OverlayCoord{u8(margin), 3});
            title_->assign(str->c_str(),
                           Text::OptColors{{custom_color(0xFFFFFF),
                                       custom_color(0x4091AD)}});

            const char* opt_1 = "continue";
            const char* opt_2 = "new game";

            const auto len_1 = utf8::len(opt_1);
            const auto len_2 = utf8::len(opt_2);

            const auto st = calc_screen_tiles(pfrm);

            const auto spacing = (st.x - (len_1 + len_2)) / 3;

            options_[0].emplace(pfrm, OverlayCoord{
                    u8(spacing), u8(st.y - 2)
                });

            options_[0]->assign(opt_1, [&]() -> Text::OptColors {
                if (not game.persistent_data().clean_) {
                    return std::nullopt;
                } else {
                    return FontColors{
                        ColorConstant::med_blue_gray,
                        ColorConstant::rich_black};
                }
            }());

            if (game.persistent_data().clean_) {
                cursor_index_ = 1;
            }

            options_[1].emplace(pfrm, opt_2, OverlayCoord{
                    u8(spacing * 2 + len_1), u8(st.y - 2)
                });


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
