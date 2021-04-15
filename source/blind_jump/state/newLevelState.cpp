#include "globals.hpp"
#include "state_impl.hpp"


const char* locale_repr_smallnum(u8 num, std::array<char, 40>& buffer);


void NewLevelState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.screen().fade(1.f);

    pfrm.load_overlay_texture("overlay");

    std::get<BlindJumpGlobalData>(globals()).visited_.clear();
}


StatePtr NewLevelState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    auto zone = zone_info(next_level_);
    auto last_zone = zone_info(next_level_ - 1);

    const bool bigfont = locale_requires_doublesize_font();

    switch (display_mode_) {
    case DisplayMode::wait_1:
        timer_ += delta;
        if (timer_ > seconds(1) / 4) {
            timer_ = 0;
            display_mode_ = DisplayMode::wait_2;

            const auto s_tiles = calc_screen_tiles(pfrm);

            auto zone = zone_info(next_level_);
            auto last_zone = zone_info(next_level_ - 1);
            if (not(zone == last_zone) or next_level_ == 0) {

                pos_ = OverlayCoord{1, u8(s_tiles.y * 0.3f)};

                if (bigfont) {
                    pos_.y -= 1;
                }

                FontConfiguration font_conf;
                font_conf.double_size_ = bigfont;

                {
                    const auto l1str = locale_string(pfrm, zone.title_line_1);
                    auto margin = centered_text_margins(
                        pfrm, utf8::len(l1str->c_str()) * (bigfont ? 2 : 1));

                    auto temp = pos_;
                    temp.x = margin;

                    text_[0].emplace(pfrm, temp, font_conf);
                    text_[0]->append(l1str->c_str());
                }

                pos_.y += 2;

                if (bigfont) {
                    pos_.y += 1;
                }

                {
                    const auto l2str = locale_string(pfrm, zone.title_line_2);
                    auto margin = centered_text_margins(
                        pfrm, utf8::len(l2str->c_str()) * (bigfont ? 2 : 1));

                    auto temp = pos_;
                    temp.x = margin;

                    text_[1].emplace(pfrm, temp, font_conf);

                    text_[1]->append(l2str->c_str());
                }

                pfrm.sleep(5);

            } else {
                FontConfiguration font_conf;
                font_conf.double_size_ = bigfont;

                text_[0].emplace(
                    pfrm,
                    OverlayCoord{1, u8(s_tiles.y - (bigfont ? 3 : 2))},
                    font_conf);

                text_[0]->append(
                    locale_string(pfrm, LocaleString::waypoint_text)->c_str());

                std::array<char, 40> buf;
                text_[0]->append(locale_repr_smallnum(next_level_, buf));
            }
        }
        break;


    case DisplayMode::wait_2:
        if (not(zone == last_zone) or next_level_ == 0) {

            timer_ += delta;

            const auto max_j =
                std::max(((int)utf8::len(
                              locale_string(pfrm, zone.title_line_2)->c_str()) *
                          (bigfont ? 2 : 1)) /
                                 2 +
                             1,
                         ((int)utf8::len(
                              locale_string(pfrm, zone.title_line_1)->c_str()) *
                          (bigfont ? 2 : 1)) /
                                 2 +
                             1);
            const auto max_i = max_j * 8;

            const int i = ease_out(timer_, 0, max_i, seconds(1));

            auto repaint = [&pfrm, this, &bigfont](int max_i) {
                while (true) {
                    int i = 0, j = 0;
                    auto center = calc_screen_tiles(pfrm).x / 2 - 1;

                    while (true) {
                        int y_off = 3;

                        if (bigfont) {
                            y_off = 4;
                        }

                        int y_lower = 2;

                        if (bigfont) {
                            y_lower = 3;
                        }

                        if (max_i > i + 7 + j * 8) {
                            pfrm.set_tile(Layer::overlay,
                                          center - j,
                                          pos_.y - y_off,
                                          107);
                            pfrm.set_tile(Layer::overlay,
                                          center - j,
                                          pos_.y + y_lower,
                                          107);

                            pfrm.set_tile(Layer::overlay,
                                          center + 1 + j,
                                          pos_.y - y_off,
                                          107);
                            pfrm.set_tile(Layer::overlay,
                                          center + 1 + j,
                                          pos_.y + y_lower,
                                          107);

                            i = 0;
                            j += 1;
                            continue;
                        }

                        if (j * 8 + i > max_i) {
                            return;
                        }

                        pfrm.set_tile(
                            Layer::overlay, center - j, pos_.y - y_off, 93 + i);
                        pfrm.set_tile(Layer::overlay,
                                      center - j,
                                      pos_.y + y_lower,
                                      93 + i);

                        pfrm.set_tile(Layer::overlay,
                                      center + 1 + j,
                                      pos_.y - y_off,
                                      100 + i);
                        pfrm.set_tile(Layer::overlay,
                                      center + 1 + j,
                                      pos_.y + y_lower,
                                      100 + i);

                        i++;

                        if (i == 8) {
                            pfrm.set_tile(Layer::overlay,
                                          center - j,
                                          pos_.y - y_off,
                                          107);
                            pfrm.set_tile(Layer::overlay,
                                          center - j,
                                          pos_.y + y_lower,
                                          107);

                            pfrm.set_tile(Layer::overlay,
                                          center + 1 + j,
                                          pos_.y - y_off,
                                          107);
                            pfrm.set_tile(Layer::overlay,
                                          center + 1 + j,
                                          pos_.y + y_lower,
                                          107);

                            i = 0;
                            j++;
                        }

                        if (j * 8 + i >= max_i) {
                            return;
                        }
                    }
                }
            };

            repaint(std::min(max_i, i));

            if (timer_ > seconds(1)) {
                pfrm.sleep(80);

                pfrm.speaker().play_music(zone.music_name_, zone.music_offset_);

                return state_pool().create<FadeInState>(game);
            }

        } else {
            timer_ += delta;

            if (timer_ > seconds(1)) {

                if (not pfrm.speaker().is_music_playing(zone.music_name_)) {
                    pfrm.speaker().play_music(zone.music_name_,
                                              zone.music_offset_);
                }

                return state_pool().create<FadeInState>(game);
            }
        }
        break;
    }

    return null_state();
}


void NewLevelState::exit(Platform& pfrm, Game& game, State&)
{
    game.next_level(pfrm, next_level_);

    // Because generating a level takes quite a bit of time (relative to a
    // normal game update step), and because we aren't really running any game
    // logic during level generation, makes sense to zero-out the delta clock,
    // otherwise the game will go into the next update cycle in the new state
    // with a huge delta time.
    pfrm.delta_clock().reset();

    text_[0].reset();
    text_[1].reset();

    pfrm.fill_overlay(0);
}
