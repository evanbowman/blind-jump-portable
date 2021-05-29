#include "state_impl.hpp"


void MapSystemState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.screen().fade(1.f);

    const auto dname = pfrm.device_name();
    map_enter_duration_ = milliseconds(280);
}


void MapSystemState::exit(Platform& pfrm, Game& game, State&)
{
    pfrm.screen().fade(1.f);
    pfrm.fill_overlay(0);
    level_text_.reset();
    for (auto& text : legend_text_) {
        text.reset();
    }
    legend_border_.reset();
}


const char* locale_repr_smallnum(u8 num, std::array<char, 40>& buffer);


StatePtr MapSystemState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    auto screen_tiles = calc_screen_tiles(pfrm);

    auto set_tile = [&](s8 x, s8 y, int icon, bool dodge = true) {
        const auto tile = pfrm.get_tile(Layer::overlay, x + 1, y);
        if (dodge and (tile == 133 or tile == 132)) {
            // ...
        } else {
            pfrm.set_tile(Layer::overlay, x + 1, y, icon);
        }
    };

    switch (anim_state_) {
    case AnimState::map_enter: {
        timer_ += delta;

        if (draw_minimap(pfrm,
                         game,
                         Float(timer_) / map_enter_duration_,
                         last_column_,
                         1,
                         0,
                         0,
                         0,
                         false)) {
            timer_ = 0;
            anim_state_ = AnimState::wp_text;
        }
        break;
    }

    case AnimState::wp_text:
        timer_ += delta;
        if (timer_ > milliseconds(32)) {
            timer_ = 0;
            anim_state_ = AnimState::legend;

            const bool bigfont = locale_requires_doublesize_font();

            FontConfiguration font_conf;
            font_conf.double_size_ = bigfont;

            auto level_str = locale_string(pfrm, LocaleString::waypoint_text);

            const u8 start_y = bigfont ? 0 : 1;

            std::array<char, 40> buffer;
            const char* level_num_str =
                locale_repr_smallnum(game.level(), buffer);

            level_text_.emplace(
                pfrm,
                OverlayCoord{
                    (utf8::len(level_str->c_str()) + utf8::len(level_num_str) ==
                     13)
                        ? u8(screen_tiles.x - 13)
                        : u8(screen_tiles.x -
                             std::min((size_t)12,
                                      (1 + utf8::len(level_str->c_str()) +
                                       utf8::len(level_num_str)) *
                                          (bigfont ? 2 : 1))),
                    start_y},
                font_conf);
            level_text_->assign(level_str->c_str());
            level_text_->append(level_num_str);
        }
        break;

    case AnimState::legend:
        timer_ += delta;
        if (timer_ > milliseconds(32)) {
            timer_ = 0;
            anim_state_ = AnimState::path_wait;

            // sigh...
            if (locale_requires_doublesize_font()) {
                set_tile(TileMap::width + 2, 4, 137, false);  // you
                set_tile(TileMap::width + 2, 7, 135, false);  // enemy
                set_tile(TileMap::width + 2, 10, 136, false); // transporter
                set_tile(TileMap::width + 2, 13, 134, false); // item
                set_tile(TileMap::width + 2, 16, 393, false); // shop

                legend_border_.emplace(pfrm,
                                       OverlayCoord{11, 16},
                                       OverlayCoord{TileMap::width + 2, 3},
                                       false,
                                       8);

                FontConfiguration font_conf;
                font_conf.double_size_ = locale_requires_doublesize_font();

                for (size_t i = 0; i < legend_strings.size(); ++i) {
                    const u8 y = 4 + (i * 3);
                    legend_text_[i].emplace(
                        pfrm,
                        locale_string(pfrm, legend_strings[i])->c_str(),
                        OverlayCoord{TileMap::width + 5, y},
                        font_conf);
                }

            } else {
                set_tile(TileMap::width + 2, 9, 137, false);  // you
                set_tile(TileMap::width + 2, 11, 135, false); // enemy
                set_tile(TileMap::width + 2, 13, 136, false); // transporter
                set_tile(TileMap::width + 2, 15, 134, false); // item
                set_tile(TileMap::width + 2, 17, 393, false); // shop

                legend_border_.emplace(pfrm,
                                       OverlayCoord{11, 11},
                                       OverlayCoord{TileMap::width + 2, 8},
                                       false,
                                       8);

                for (size_t i = 0; i < legend_strings.size(); ++i) {
                    const u8 y = 9 + (i * 2);
                    legend_text_[i].emplace(
                        pfrm,
                        locale_string(pfrm, legend_strings[i])->c_str(),
                        OverlayCoord{TileMap::width + 5, y});
                }
            }

            path_finder_.emplace(allocate_dynamic<IncrementalPathfinder>(
                pfrm,
                pfrm,
                game.tiles(),
                get_constrained_player_tile_coord(game).cast<u8>(),
                to_tile_coord(game.transporter().get_position().cast<s32>())
                    .cast<u8>()));
        }
        break;

    case AnimState::path_wait: {
        if (pfrm.keyboard().down_transition(game.action2_key())) {
            return state_pool().create<InventoryState>(false);
        }

        if (not path_finder_) {
            anim_state_ = AnimState::wait;
        }

        bool incomplete = true;

        if (auto result = path_finder_->obj_->compute(pfrm, 8, &incomplete)) {
            path_ = std::move(result);
            anim_state_ = AnimState::wait;
            path_finder_.reset();

            auto* const path = [&]() -> PathBuffer* {
                if (path_) {
                    return path_->obj_.get();
                } else {
                    return nullptr;
                }
            }();

            draw_minimap(
                pfrm, game, 1.f, last_column_ = -1, 1, 0, 0, 0, false, path);
        }

        if (incomplete == false) {
            anim_state_ = AnimState::wait;
            path_finder_.reset();
        }
        break;
    }


    case AnimState::wait:
        if (pfrm.keyboard().down_transition(game.action2_key())) {
            return state_pool().create<InventoryState>(false);
        }
        break;
    }

    return null_state();
}
