#include "state_impl.hpp"


void PauseScreenState::draw_cursor_image(Platform& pfrm,
                                         Text* target,
                                         int tile1,
                                         int tile2)
{
    const bool bigfont = locale_requires_doublesize_font();

    const auto pos = target->coord();
    pfrm.set_tile(Layer::overlay, pos.x - 2, pos.y + (bigfont ? 1 : 0), tile1);
    pfrm.set_tile(Layer::overlay,
                  pos.x + (target->len() * (bigfont ? 2 : 1)) + 1,
                  pos.y + (bigfont ? 1 : 0),
                  tile2);
}


void PauseScreenState::erase_cursor(Platform& pfrm)
{
    for (auto& text : texts_) {
        draw_cursor_image(pfrm, &text, 0, 0);
    }
}


void PauseScreenState::draw_cursor(Platform& pfrm)
{
    if (texts_.empty()) {
        return;
    }

    auto [left, right] = [&]() -> Vec2<int> {
        switch (anim_index_) {
        default:
        case 0:
            return {373, 374};
        case 1:
            return {375, 376};
        }
    }();

    erase_cursor(pfrm);

    if (cursor_loc_ < (int)texts_.size()) {
        draw_cursor_image(pfrm, &texts_[cursor_loc_], left, right);
    } else {
        // error!
    }
}


void PauseScreenState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    if (dynamic_cast<ActiveState*>(&prev_state)) {
        cursor_loc_ = 0;
    }

    strs_.push_back(LocaleString::menu_resume);
    strs_.push_back(LocaleString::menu_settings);
    strs_.push_back(LocaleString::menu_connect_peer);
    strs_.push_back(LocaleString::menu_save_and_quit);
}


bool PauseScreenState::connect_peer_option_available(Game& game) const
{
    // For now, don't allow syncing when someone's progressed through any of the
    // levels.
    return game.level() == Level{0} and
           Platform::NetworkPeer::supported_by_device();
}


void PauseScreenState::repaint_text(Platform& pfrm, Game& game)
{
    const auto screen_tiles = calc_screen_tiles(pfrm);

    const u8 y = screen_tiles.y / 2;


    const bool bigfont = locale_requires_doublesize_font();


    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;


    for (int i = 0; i < (int)strs_.size(); ++i) {
        const auto len = utf8::len(locale_string(pfrm, strs_[i])->c_str()) *
                         (bigfont ? 2 : 1);

        const u8 text_x_loc = (screen_tiles.x - len) / 2;

        texts_.emplace_back(
            pfrm,
            OverlayCoord{text_x_loc,
                         u8(y - strs_.size() + i * (bigfont ? 3 : 2) +
                            (bigfont ? -1 : 0))},
            font_conf);

        if (strs_[i] == LocaleString::menu_connect_peer) {
            texts_.back().assign(locale_string(pfrm, strs_[i])->c_str(),
                                 [&]() -> Text::OptColors {
                                     if (connect_peer_option_available(game)) {
                                         return std::nullopt;
                                     } else {
                                         return FontColors{
                                             ColorConstant::med_blue_gray,
                                             ColorConstant::rich_black};
                                     }
                                 }());
        } else {
            texts_.back().assign(locale_string(pfrm, strs_[i])->c_str());
        }
    }
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

            repaint_text(pfrm, game);

            draw_cursor(pfrm);
        }

        pfrm.screen().fade(smoothstep(0.f, fade_duration, fade_timer_));
    } else {

        const auto& line = texts_[cursor_loc_];
        const auto y_center = pfrm.screen().size().y / 2;
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

        if (pfrm.keyboard().down_transition<Key::down>() and
            cursor_loc_ < int(texts_.size() - 1)) {

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
            switch (strs_[cursor_loc_]) {
            case LocaleString::menu_resume:
                return state_pool().create<ActiveState>();

            case LocaleString::menu_settings:
                return state_pool().create<EditSettingsState>(
                    make_deferred_state<PauseScreenState>(false));

            case LocaleString::menu_console:
                return state_pool().create<LispReplState>();

            case LocaleString::menu_connect_peer:
                if (connect_peer_option_available(game)) {
                    return state_pool().create<NetworkConnectSetupState>();
                }
                break;

            case LocaleString::menu_save_and_quit:
                return state_pool().create<GoodbyeState>();

            default:
                // FIXME: how'd we receive an unexpected string here?
                break;
            }
        } else if (pfrm.keyboard().down_transition<Key::start>()) {
            cursor_loc_ = 0;
            return state_pool().create<ActiveState>();
        } else if (pfrm.keyboard().down_transition<Key::alt_1>()) {
            if (++developer_mode_activation_counter_ == 7) {
                erase_cursor(pfrm);
                texts_.clear();
                strs_.clear();
                strs_.push_back(LocaleString::menu_resume);
                strs_.push_back(LocaleString::menu_settings);
                strs_.push_back(LocaleString::menu_connect_peer);
                strs_.push_back(LocaleString::menu_console); // added
                strs_.push_back(LocaleString::menu_save_and_quit);

                repaint_text(pfrm, game);
            }
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
    texts_.clear();

    pfrm.fill_overlay(0);
}
