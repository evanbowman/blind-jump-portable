#include "state_impl.hpp"


void QuickSelectInventoryState::display_time_remaining(Platform&, Game&)
{
}


void QuickSelectInventoryState::enter(Platform& pfrm,
                                      Game& game,
                                      State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);

    sidebar_.emplace(pfrm,
                     6,
                     calc_screen_tiles(pfrm).y,
                     OverlayCoord{calc_screen_tiles(pfrm).x, 0});
    sidebar_->set_display_percentage(0.f);

    repaint_powerups(pfrm,
                     game,
                     true,
                     &health_,
                     &score_,
                     nullptr,
                     &powerups_,
                     UIMetric::Align::left);

    game.camera().set_speed(2.8f);
}


void QuickSelectInventoryState::exit(Platform& pfrm,
                                     Game& game,
                                     State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);

    health_.reset();
    score_.reset();
    powerups_.clear();
    item_icons_.clear();

    sidebar_.reset();
    selector_.reset();
    sidebar_->set_display_percentage(0.f);

    game.camera().set_speed(1.f);
}


static bool item_is_quickselect(Item::Type item)
{
    if (auto handler = inventory_item_handler(item)) {
        return handler->single_use_ and
               item not_eq Item::Type::signal_jammer and
               item not_eq Item::Type::long_jump_z2 and
               item not_eq Item::Type::long_jump_z3 and
               item not_eq Item::Type::long_jump_z4;
    } else {
        return false;
    }
}


template <typename F> void foreach_quickselect_item(Game& game, F&& callback)
{
    for (int page = 0; page < Inventory::pages; ++page) {
        for (int row = 0; row < Inventory::rows; ++row) {
            for (int col = 0; col < Inventory::cols; ++col) {
                const auto item = game.inventory().get_item(page, col, row);
                if (item_is_quickselect(item)) {
                    callback(item, page, row, col);
                }
            }
        }
    }
}


void QuickSelectInventoryState::draw_items(Platform& pfrm, Game& game)
{
    items_.clear();
    item_icons_.clear();

    more_pages_ = false;

    auto screen_tiles = calc_screen_tiles(pfrm);

    int skip = page_ * items_.capacity();

    auto margin = [&] {
        if (screen_tiles.y == 17) {
            return 0;
        } else {
            return 1;
        }
    }();

    foreach_quickselect_item(game, [&](Item::Type item, int, int, int) {
        if (item_icons_.full()) {
            more_pages_ = true;
            return;
        }
        if (skip > 0) {
            --skip;
            return;
        }
        items_.push_back(item);

        const OverlayCoord coord{
            static_cast<u8>(screen_tiles.x - 4),
            static_cast<u8>(3 + margin + item_icons_.size() * 5)};

        if (auto handler = inventory_item_handler(item)) {
            item_icons_.emplace_back(pfrm, handler->icon_, coord);
        }
    });
}


void QuickSelectInventoryState::show_sidebar(Platform& pfrm)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    sidebar_->set_display_percentage(1.f);

    auto margin = [&] {
        if (screen_tiles.y == 17) {
            return 0;
        } else {
            return 1;
        }
    }();

    for (int y = 0; y < screen_tiles.y; y += 5) {
        pfrm.set_tile(Layer::overlay, screen_tiles.x - 1, y + 1 + margin, 129);
        pfrm.set_tile(Layer::overlay, screen_tiles.x - 6, y + 1 + margin, 129);
    }

    if (page_ > 0) {
        pfrm.set_tile(Layer::overlay, screen_tiles.x - 3, margin, 153);
    } else {
        pfrm.set_tile(Layer::overlay, screen_tiles.x - 3, margin, 151);
    }

    if (more_pages_) {
        pfrm.set_tile(Layer::overlay,
                      screen_tiles.x - 3,
                      screen_tiles.y - (1 + margin),
                      154);
    } else {
        pfrm.set_tile(Layer::overlay,
                      screen_tiles.x - 3,
                      screen_tiles.y - (1 + margin),
                      152);
    }
}


StatePtr QuickSelectInventoryState::update(Platform& pfrm,
                                           Game& game,
                                           Microseconds delta)
{
    // We can end up with graphical glitches if we allow the notification bar to
    // play its exit animation while in the one of the sidebar states, so keep
    // the notification text alive until we exit this state.
    if (notification_status == NotificationStatus::display) {
        notification_text_timer = seconds(3);
    }

    const auto last_health = game.player().get_health();
    const auto last_score = game.score();

    const auto player_pos = game.player().get_position();

    game.camera().push_ballast({player_pos.x + 20, player_pos.y});

    if (auto new_state = OverworldState::update(pfrm, game, delta)) {
        return new_state;
    }

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


    static const auto transition_duration = milliseconds(160);

    auto redraw_selector = [&](TileDesc erase_color) {
        auto screen_tiles = calc_screen_tiles(pfrm);

        auto margin = [&] {
            if (screen_tiles.y == 17) {
                return 0;
            } else {
                return 1;
            }
        }();

        const OverlayCoord pos{static_cast<u8>(screen_tiles.x - 5),
                               static_cast<u8>(2 + margin + selector_pos_ * 5)};
        if (selector_shaded_) {
            selector_.emplace(
                pfrm, OverlayCoord{4, 4}, pos, false, 8 + 278, erase_color);
            selector_shaded_ = false;
        } else {
            selector_.emplace(
                pfrm, OverlayCoord{4, 4}, pos, false, 16 + 278, erase_color);
            selector_shaded_ = true;
        }
    };


    switch (display_mode_) {
    case DisplayMode::enter: {
        timer_ += delta;

        game.player().update(pfrm, game, delta);

        if (timer_ >= transition_duration) {
            timer_ = 0;
            display_mode_ = DisplayMode::show;

            restore_keystates = pfrm.keyboard().dump_state();

            draw_items(pfrm, game);
            show_sidebar(pfrm);
            draw_items(pfrm, game);

        } else {
            if (not pfrm.keyboard().pressed<quick_select_inventory_key>()) {
                timer_ = transition_duration - timer_;
                display_mode_ = DisplayMode::exit;
                hide_notifications(pfrm);
            } else {
                const auto amount =
                    smoothstep(0.f, transition_duration, timer_);

                sidebar_->set_display_percentage(amount);
                // pfrm.screen().fade(0.25f * amount);
            }
        }
        break;
    }

    case DisplayMode::show: {

        timer_ += delta;

        game.player().soft_update(pfrm, game, delta);

        if (pfrm.keyboard().down_transition<Key::up>()) {
            if (selector_pos_ > 0) {
                pfrm.speaker().play_sound("scroll", 1);
                selector_pos_ -= 1;
            } else if (page_ > 0) {
                pfrm.speaker().play_sound("scroll", 1);
                page_ -= 1;
                selector_pos_ = items_.capacity() - 1;
                draw_items(pfrm, game);
                show_sidebar(pfrm);
                draw_items(pfrm, game);
            }
            redraw_selector(112);
        } else if (pfrm.keyboard().down_transition<Key::down>()) {
            if (selector_pos_ < items_.capacity() - 1) {
                selector_pos_ += 1;
                pfrm.speaker().play_sound("scroll", 1);
            } else {
                if (more_pages_) {
                    pfrm.speaker().play_sound("scroll", 1);
                    selector_pos_ = 0;
                    page_ += 1;
                    draw_items(pfrm, game);
                    show_sidebar(pfrm);
                    draw_items(pfrm, game);
                }
            }
            redraw_selector(112);
        } else if (pfrm.keyboard().down_transition(game.action2_key()) or
                   pfrm.keyboard().down_transition(game.action1_key())) {
            if (selector_pos_ < items_.size()) {
                int skip = page_ * items_.capacity() + selector_pos_;
                bool done = false;
                StatePtr next_state = null_state();
                foreach_quickselect_item(
                    game, [&](Item::Type item, int page, int row, int col) {
                        if (skip > 0) {
                            --skip;
                            return;
                        }
                        if (done) {
                            return;
                        }
                        if (pfrm.keyboard().down_transition(
                                game.action2_key())) {
                            if (auto handler = inventory_item_handler(item)) {
                                next_state = handler->callback_(pfrm, game);
                                game.inventory().remove_item(page, col, row);
                                done = true;

                                if (item not_eq items_[selector_pos_]) {
                                    pfrm.fatal("quick sel inv logic err");
                                }
                            }
                            // NOTE: boss level spritesheets do not necessarily
                            // include the sprites for item chests.
                        } else if (not is_boss_level(game.level())) {
                            done = true;
                            // Intended for sharing items in multiplayer
                            // mode. Drop an item chest, send a message to the
                            // other game.
                            if (share_item(pfrm,
                                           game,
                                           game.player().get_position(),
                                           items_[selector_pos_])) {
                                pfrm.speaker().play_sound("dropitem", 3);
                                game.inventory().remove_item(page, col, row);
                            }
                        }
                    });

                if (next_state) {
                    return next_state;
                }

                while (not selector_shaded_)
                    redraw_selector(112);

                display_mode_ = DisplayMode::used_item;
                timer_ = 0;
                break;
            }
        }

        if (timer_ > milliseconds(75)) {
            timer_ = 0;
            redraw_selector(112);
        }

        if (not pfrm.keyboard().pressed<quick_select_inventory_key>()) {
            display_mode_ = DisplayMode::exit;
            timer_ = 0;
            redraw_selector(0);
            hide_notifications(pfrm);

            if (restore_keystates) {
                pfrm.keyboard().restore_state(*restore_keystates);
                restore_keystates.reset();
            }
        }
        break;
    }

    case DisplayMode::used_item: {
        timer_ += delta;

        if (not pfrm.keyboard().pressed<quick_select_inventory_key>()) {
            display_mode_ = DisplayMode::exit;
            timer_ = 0;
            redraw_selector(0);
            hide_notifications(pfrm);

            if (restore_keystates) {
                pfrm.keyboard().restore_state(*restore_keystates);
                restore_keystates.reset();
            }
        }

        if (timer_ > milliseconds(32)) {
            timer_ = 0;

            if (used_item_anim_index_ < 6) {
                used_item_anim_index_ += 1;

                auto screen_tiles = calc_screen_tiles(pfrm);

                auto margin = [&] {
                    if (screen_tiles.y == 17) {
                        return 0;
                    } else {
                        return 1;
                    }
                }();

                const OverlayCoord pos{
                    static_cast<u8>(screen_tiles.x - 5),
                    static_cast<u8>(2 + margin + selector_pos_ * 5)};

                const auto idx = 394 + used_item_anim_index_;

                pfrm.set_tile(Layer::overlay, pos.x + 1, pos.y + 1, idx);
                pfrm.set_tile(Layer::overlay, pos.x + 2, pos.y + 1, idx);
                pfrm.set_tile(Layer::overlay, pos.x + 1, pos.y + 2, idx);
                pfrm.set_tile(Layer::overlay, pos.x + 2, pos.y + 2, idx);

            } else {
                used_item_anim_index_ = 0;

                draw_items(pfrm, game);
                show_sidebar(pfrm); // Because MediumIcon resets tiles back to
                // zero, thus punching a hole in the
                // sidebar.
                draw_items(pfrm, game);


                repaint_powerups(pfrm,
                                 game,
                                 true,
                                 &health_,
                                 &score_,
                                 nullptr,
                                 &powerups_,
                                 UIMetric::Align::left);
                redraw_selector(112);

                display_mode_ = DisplayMode::show;
            }
        }
        break;
    }


    case DisplayMode::exit: {
        game.player().update(pfrm, game, delta);

        timer_ += delta;
        if (timer_ >= transition_duration) {
            timer_ = 0;
            return state_pool().create<ActiveState>();
        } else {
            // if (pfrm.keyboard().pressed<quick_select_inventory_key>()) {
            //     timer_ = transition_duration - timer_;
            //     display_mode_ = DisplayMode::enter;
            // } else {
            const auto amount =
                1.f - smoothstep(0.f, transition_duration, timer_);

            sidebar_->set_display_percentage(amount);
            // pfrm.screen().fade(0.25f * amount);
            // }
        }
        break;
    }
    }


    return null_state();
}
