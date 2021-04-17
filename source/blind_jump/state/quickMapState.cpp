#include "state_impl.hpp"


void QuickMapState::display_time_remaining(Platform&, Game&)
{
}


void QuickMapState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);

    sidebar_.emplace(pfrm, 15, calc_screen_tiles(pfrm).y, OverlayCoord{});
    sidebar_->set_display_percentage(0.f);

    repaint_powerups(pfrm,
                     game,
                     true,
                     &health_,
                     &score_,
                     nullptr,
                     &powerups_,
                     UIMetric::Align::right);

    game.camera().set_speed(2.9f);
}


void QuickMapState::exit(Platform& pfrm, Game& game, State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);

    sidebar_.reset();
    sidebar_->set_display_percentage(0.f);

    powerups_.clear();
    health_.reset();
    score_.reset();

    level_text_.reset();

    game.camera().set_speed(1.f);
}


const char* locale_repr_smallnum(u8 num, std::array<char, 40>& buffer);


StatePtr QuickMapState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    // We can end up with graphical glitches if we allow the notification bar to
    // play its exit animation while in the one of the sidebar states, so keep
    // the notification text alive until we exit this state.
    if (notification_status == NotificationStatus::display) {
        notification_text_timer = seconds(3);
    }

    const auto player_pos = game.player().get_position();

    game.camera().push_ballast({player_pos.x - 80, player_pos.y});

    const auto last_health = game.player().get_health();
    const auto last_score = game.score();

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
                      UIMetric::Align::right);


    static const auto transition_duration = milliseconds(220);

    switch (display_mode_) {
    case DisplayMode::enter: {
        timer_ += delta;

        game.player().update(pfrm, game, delta);

        if (timer_ >= transition_duration) {
            timer_ = 0;
            display_mode_ = DisplayMode::draw;


            sidebar_->set_display_percentage(0.96f);

            if (resolution(pfrm.screen()) not_eq Resolution::r16_9) {

                const bool bigfont = locale_requires_doublesize_font();

                FontConfiguration font_conf;
                font_conf.double_size_ = bigfont;


                std::array<char, 40> buffer;
                const char* level_num_str =
                    locale_repr_smallnum(game.level(), buffer);


                auto level_str =
                    locale_string(pfrm, LocaleString::waypoint_text);
                const auto text_len =
                    (utf8::len(level_str->c_str()) + utf8::len(level_num_str)) *
                    (bigfont ? 2 : 1);

                level_text_.emplace(
                    pfrm, OverlayCoord{u8((15 - text_len) / 2), 0}, font_conf);

                level_text_->assign(level_str->c_str());
                level_text_->append(level_num_str);
            }

            restore_keystates = pfrm.keyboard().dump_state();

        } else {
            if (not pfrm.keyboard().pressed<quick_map_key>()) {
                timer_ = transition_duration - timer_;
                display_mode_ = DisplayMode::exit;
                hide_notifications(pfrm);
            } else {
                const auto amount =
                    0.96f * smoothstep(0.f, transition_duration, timer_);

                sidebar_->set_display_percentage(amount);
            }
        }
        break;
    }

    case DisplayMode::draw: {

        game.player().soft_update(pfrm, game, delta);


        timer_ += delta;

        const auto duration = milliseconds(180);

        if (timer_ >= duration) {
            timer_ = 0;
            display_mode_ = DisplayMode::path_wait;

            path_finder_.emplace(allocate_dynamic<IncrementalPathfinder>(
                pfrm,
                pfrm,
                game.tiles(),
                get_constrained_player_tile_coord(game).cast<u8>(),
                to_tile_coord(game.transporter().get_position().cast<s32>())
                    .cast<u8>()));

            int offset = 0;
            if (resolution(pfrm.screen()) == Resolution::r16_9) {
                offset = -1;
            }

            const bool bigfont = locale_requires_doublesize_font();
            if (bigfont) {
                offset += 1;
            }

            draw_minimap(
                pfrm, game, 0.9f, last_map_column_, -1, offset, 1, 1, true);
        } else {

            int offset = 0;
            if (resolution(pfrm.screen()) == Resolution::r16_9) {
                offset = -1;
            }

            const bool bigfont = locale_requires_doublesize_font();
            if (bigfont) {
                offset += 1;
            }

            draw_minimap(pfrm,
                         game,
                         0.9f * (timer_ / Float(duration)),
                         last_map_column_,
                         -1,
                         offset,
                         1,
                         1);
        }

        break;
    }

    case DisplayMode::path_wait: {

        bool incomplete = true;

        if (auto result = path_finder_->obj_->compute(pfrm, 8, &incomplete)) {
            path_ = std::move(result);
            display_mode_ = DisplayMode::show;
            path_finder_.reset();
        }

        if (incomplete == false) {
            display_mode_ = DisplayMode::show;
            path_finder_.reset();
        }

        if (not pfrm.keyboard().pressed<quick_map_key>()) {
            display_mode_ = DisplayMode::exit;
            timer_ = 0;
            hide_notifications(pfrm);

            if (restore_keystates) {
                pfrm.keyboard().restore_state(*restore_keystates);
                restore_keystates.reset();
            }
        }
        break;
    }

    case DisplayMode::show: {

        timer_ += delta;

        if (timer_ > seconds(1) + milliseconds(60)) {
            timer_ = 0;
            last_map_column_ = -1;
            auto* const path = [&]() -> PathBuffer* {
                if (path_) {
                    return path_->obj_.get();
                } else {
                    return nullptr;
                }
            }();
            int offset = 0;
            if (resolution(pfrm.screen()) == Resolution::r16_9) {
                offset = -1;
            }

            const bool bigfont = locale_requires_doublesize_font();
            if (bigfont) {
                offset += 1;
            }

            draw_minimap(pfrm,
                         game,
                         0.9f,
                         last_map_column_,
                         -1,
                         offset,
                         1,
                         1,
                         true,
                         path);
        }

        game.player().soft_update(pfrm, game, delta);

        if (not pfrm.keyboard().pressed<quick_map_key>()) {
            display_mode_ = DisplayMode::exit;
            timer_ = 0;
            hide_notifications(pfrm);

            if (restore_keystates) {
                pfrm.keyboard().restore_state(*restore_keystates);
                restore_keystates.reset();
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
            // if (pfrm.keyboard().pressed<quick_map_key>()) {
            //     timer_ = transition_duration - timer_;
            //     display_mode_ = DisplayMode::enter;
            //     last_map_column_ = -1;
            // } else {
            const auto amount =
                1.f - smoothstep(0.f, transition_duration, timer_);

            sidebar_->set_display_percentage(amount);
            // }
        }
        break;
    }
    }


    return null_state();
}
