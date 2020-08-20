#include "state_impl.hpp"
#include "conf.hpp"


void MapSystemState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.screen().fade(1.f);

    const auto dname = pfrm.device_name();
    map_enter_duration_ = milliseconds(
        Conf(pfrm).expect<Conf::Integer>(dname.c_str(), "minimap_enter_time"));
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
                         last_column_)) {
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

            const char* level_str = locale_string(LocaleString::waypoint_text);

            level_text_.emplace(
                pfrm,
                OverlayCoord{
                    u8(screen_tiles.x - (1 + utf8::len(level_str) +
                                         integer_text_length(game.level()))),
                    1});
            level_text_->assign(level_str);
            level_text_->append(game.level());
        }
        break;

    case AnimState::legend:
        timer_ += delta;
        if (timer_ > milliseconds(32)) {
            timer_ = 0;
            anim_state_ = AnimState::path_wait;

            set_tile(TileMap::width + 2, 11, 137, false); // you
            set_tile(TileMap::width + 2, 13, 135, false); // enemy
            set_tile(TileMap::width + 2, 15, 136, false); // transporter
            set_tile(TileMap::width + 2, 17, 134, false); // item

            legend_border_.emplace(pfrm,
                                   OverlayCoord{11, 9},
                                   OverlayCoord{TileMap::width + 2, 10},
                                   false,
                                   8);

            for (size_t i = 0; i < legend_strings.size(); ++i) {
                const u8 y = 11 + (i * 2);
                legend_text_[i].emplace(pfrm,
                                        locale_string(legend_strings[i]),
                                        OverlayCoord{TileMap::width + 5, y});
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
        if (pfrm.keyboard().down_transition<Key::action_2>()) {
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
                pfrm, game, 1.f, last_column_ = -1, 1, 0, 0, false, path);
        }

        if (incomplete == false) {
            anim_state_ = AnimState::wait;
            path_finder_.reset();
        }
        break;
    }


    case AnimState::wait:
        if (pfrm.keyboard().down_transition<Key::action_2>()) {
            return state_pool().create<InventoryState>(false);
        }
        break;
    }

    return null_state();
}
