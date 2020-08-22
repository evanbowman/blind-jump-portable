#include "state_impl.hpp"


void PauseScreenState::draw_cursor(Platform& pfrm)
{
    // draw_dot_grid(pfrm);

    if (not resume_text_ or not save_and_quit_text_ or not settings_text_ or
        not connect_peer_text_) {
        return;
    }

    auto draw_cursor = [&pfrm](Text* target, int tile1, int tile2) {
        const auto pos = target->coord();
        pfrm.set_tile(Layer::overlay, pos.x - 2, pos.y, tile1);
        pfrm.set_tile(Layer::overlay, pos.x + target->len() + 1, pos.y, tile2);
    };

    auto [left, right] = [&]() -> Vec2<int> {
        switch (anim_index_) {
        default:
        case 0:
            return {147, 148};
        case 1:
            return {149, 150};
        }
    }();

    switch (cursor_loc_) {
    default:
    case 0:
        draw_cursor(&(*resume_text_), left, right);
        draw_cursor(&(*connect_peer_text_), 0, 0);
        draw_cursor(&(*settings_text_), 0, 0);
        draw_cursor(&(*save_and_quit_text_), 0, 0);
        break;

    case 1:
        draw_cursor(&(*resume_text_), 0, 0);
        draw_cursor(&(*connect_peer_text_), 0, 0);
        draw_cursor(&(*settings_text_), left, right);
        draw_cursor(&(*save_and_quit_text_), 0, 0);
        break;

    case 2:
        draw_cursor(&(*resume_text_), 0, 0);
        draw_cursor(&(*connect_peer_text_), left, right);
        draw_cursor(&(*settings_text_), 0, 0);
        draw_cursor(&(*save_and_quit_text_), 0, 0);
        break;

    case 3:
        draw_cursor(&(*resume_text_), 0, 0);
        draw_cursor(&(*connect_peer_text_), 0, 0);
        draw_cursor(&(*settings_text_), 0, 0);
        draw_cursor(&(*save_and_quit_text_), left, right);
        break;
    }
}


void PauseScreenState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    if (dynamic_cast<ActiveState*>(&prev_state)) {
        cursor_loc_ = 0;
    }
}


bool PauseScreenState::connect_peer_option_available(Game& game) const
{
    // For now, don't allow syncing when someone's progressed through any of the
    // levels.
    return game.level() == Level{0} and
           Platform::NetworkPeer::supported_by_device();
}


StatePtr
PauseScreenState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (pfrm.keyboard().pressed<inventory_key>()) {
        if (restore_keystates) {
            // The existing keystates when returning to the overworld think that
            // we entered the pause screen, but now we're jumping over to the
            // inventory, so adjust key cache accordingly.
            restore_keystates->set(static_cast<int>(inventory_key), true);
            restore_keystates->set(static_cast<int>(Key::start), false);
        }
        return state_pool().create<InventoryState>(false);
    }

    if (fade_timer_ < fade_duration) {
        fade_timer_ += delta;

        if (fade_timer_ >= fade_duration) {
            const auto screen_tiles = calc_screen_tiles(pfrm);

            const auto resume_text_len =
                utf8::len(locale_string(LocaleString::menu_resume));

            const auto connect_peer_text_len =
                utf8::len(locale_string(LocaleString::menu_connect_peer));

            const auto settings_text_len =
                utf8::len(locale_string(LocaleString::menu_settings));

            const auto snq_text_len =
                utf8::len(locale_string(LocaleString::menu_save_and_quit));

            const u8 resume_x_loc = (screen_tiles.x - resume_text_len) / 2;
            const u8 cp_x_loc = (screen_tiles.x - connect_peer_text_len) / 2;
            const u8 settings_x_loc = (screen_tiles.x - settings_text_len) / 2;
            const u8 snq_x_loc = (screen_tiles.x - snq_text_len) / 2;

            const u8 y = screen_tiles.y / 2;

            resume_text_.emplace(pfrm, OverlayCoord{resume_x_loc, u8(y - 4)});
            connect_peer_text_.emplace(pfrm, OverlayCoord{cp_x_loc, u8(y - 0)});
            settings_text_.emplace(pfrm,
                                   OverlayCoord{settings_x_loc, u8(y - 2)});
            save_and_quit_text_.emplace(pfrm,
                                        OverlayCoord{snq_x_loc, u8(y + 2)});

            resume_text_->assign(locale_string(LocaleString::menu_resume));

            connect_peer_text_->assign(
                locale_string(LocaleString::menu_connect_peer),
                [&]() -> Text::OptColors {
                    if (connect_peer_option_available(game)) {
                        return std::nullopt;
                    } else {
                        return FontColors{ColorConstant::med_blue_gray,
                                          ColorConstant::rich_black};
                    }
                }());

            settings_text_->assign(locale_string(LocaleString::menu_settings));
            save_and_quit_text_->assign(
                locale_string(LocaleString::menu_save_and_quit));

            draw_cursor(pfrm);
        }

        pfrm.screen().fade(smoothstep(0.f, fade_duration, fade_timer_));
    } else {

        const auto& line = [&]() -> Text& {
            switch (cursor_loc_) {
            default:
            case 0:
                return *resume_text_;
            case 1:
                return *settings_text_;
            case 2:
                return *connect_peer_text_;
            case 3:
                return *save_and_quit_text_;
            }
        }();
        const Float y_center = pfrm.screen().size().y / 2;
        const Float y_line = line.coord().y * 8;
        const auto y_diff = (y_line - y_center) * 0.4f;

        y_offset_ = interpolate(Float(y_diff), y_offset_, delta * 0.00001f);

        pfrm.set_overlay_origin(0, y_offset_);


        anim_timer_ += delta;
        if (anim_timer_ > milliseconds(75)) {
            anim_timer_ = 0;
            if (++anim_index_ > 1) {
                anim_index_ = 0;
            }
            draw_cursor(pfrm);
        }

        if (pfrm.keyboard().down_transition<Key::down>() and cursor_loc_ < 3) {
            cursor_loc_ += 1;
            pfrm.speaker().play_sound("scroll", 1);
            draw_cursor(pfrm);
        } else if (pfrm.keyboard().down_transition<Key::up>() and
                   cursor_loc_ > 0) {
            cursor_loc_ -= 1;
            pfrm.speaker().play_sound("scroll", 1);
            draw_cursor(pfrm);
        } else if (pfrm.keyboard().down_transition(game.action2_key())) {
            pfrm.speaker().play_sound("select", 1);
            switch (cursor_loc_) {
            case 0:
                return state_pool().create<ActiveState>(game);
            case 1:
                return state_pool().create<EditSettingsState>(make_deferred_state<PauseScreenState>(false));
            case 2:
                if (connect_peer_option_available(game)) {
                    return state_pool().create<NetworkConnectSetupState>();
                }
                break;
            case 3:
                return state_pool().create<GoodbyeState>();
            }
        } else if (pfrm.keyboard().down_transition<Key::start>()) {
            cursor_loc_ = 0;
            return state_pool().create<ActiveState>(game);
        }
    }

    if (pfrm.keyboard().pressed<Key::alt_1>()) {
        log_timer_ += delta;
        if (log_timer_ > seconds(1)) {
            return state_pool().create<LogfileViewerState>();
        }
    } else {
        log_timer_ = 0;
    }

    return null_state();
}


void PauseScreenState::exit(Platform& pfrm, Game& game, State& next_state)
{
    pfrm.set_overlay_origin(0, 0);

    resume_text_.reset();
    connect_peer_text_.reset();
    settings_text_.reset();
    save_and_quit_text_.reset();

    pfrm.fill_overlay(0);
}
