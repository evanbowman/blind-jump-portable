#include "state_impl.hpp"


void EditSettingsState::message(Platform& pfrm, const char* str)
{
    str_ = str;

    auto screen_tiles = calc_screen_tiles(pfrm);

    message_anim_.emplace(pfrm, OverlayCoord{0, u8(screen_tiles.y - 1)});

    message_anim_->init(screen_tiles.x);

    pfrm.set_overlay_origin(0, 0);
}


EditSettingsState::EditSettingsState(DeferredState exit_state)
    : exit_state_(exit_state), lines_{{{swap_action_keys_line_updater_},
                                       {difficulty_line_updater_},
                                       {language_line_updater_},
                                       {contrast_line_updater_},
                                       {night_mode_line_updater_},
                                       {show_stats_line_updater_}}}
{
}


void EditSettingsState::draw_line(Platform& pfrm, int row, const char* value)
{
    const int value_len = utf8::len(value);
    const int field_len = utf8::len(locale_string(strings[row]));

    const auto margin = centered_text_margins(pfrm, value_len + field_len + 2);

    lines_[row].text_.emplace(pfrm, OverlayCoord{0, u8(4 + row * 2)});

    left_text_margin(*lines_[row].text_, margin);
    lines_[row].text_->append(locale_string(strings[row]));
    lines_[row].text_->append("  ");
    lines_[row].text_->append(value);

    lines_[row].cursor_begin_ = margin + field_len;
    lines_[row].cursor_end_ = margin + field_len + 2 + value_len + 1;
}


void EditSettingsState::refresh(Platform& pfrm, Game& game)
{
    for (u32 i = 0; i < lines_.size(); ++i) {
        draw_line(pfrm, i, lines_[i].updater_.update(pfrm, game, 0).c_str());
    }
}


void EditSettingsState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    refresh(pfrm, game);
}


void EditSettingsState::exit(Platform& pfrm, Game& game, State& next_state)
{
    for (auto& l : lines_) {
        l.text_.reset();
    }

    message_.reset();

    pfrm.fill_overlay(0);

    pfrm.set_overlay_origin(0, 0);
}


StatePtr
EditSettingsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition(game.action2_key())) {
        pfrm.speaker().play_sound("select", 1);
        return exit_state_();
    }

    if (message_anim_ and not message_anim_->done()) {
        message_anim_->update(delta);

        if (message_anim_->done()) {
            message_anim_.reset();

            auto screen_tiles = calc_screen_tiles(pfrm);
            const auto len = utf8::len(str_);
            message_.emplace(pfrm,
                             str_,
                             OverlayCoord{u8((screen_tiles.x - len) / 2),
                                          u8(screen_tiles.y - 1)});
        }
    }

    auto erase_selector = [&] {
        for (u32 i = 0; i < lines_.size(); ++i) {
            const auto& line = lines_[i];
            pfrm.set_tile(
                Layer::overlay, line.cursor_begin_, line.text_->coord().y, 0);
            pfrm.set_tile(
                Layer::overlay, line.cursor_end_, line.text_->coord().y, 0);
        }
    };

    if (pfrm.keyboard().down_transition<Key::down>()) {
        message_.reset();
        if (select_row_ < static_cast<int>(lines_.size() - 1)) {
            select_row_ += 1;
            pfrm.speaker().play_sound("scroll", 1);
        }
    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        message_.reset();
        if (select_row_ > 0) {
            select_row_ -= 1;
            pfrm.speaker().play_sound("scroll", 1);
        }
    } else if (pfrm.keyboard().down_transition<Key::right>()) {
        message_.reset();

        erase_selector();

        auto& updater = lines_[select_row_].updater_;

        draw_line(pfrm, select_row_, updater.update(pfrm, game, 1).c_str());

        updater.complete(pfrm, game, *this);

        pfrm.speaker().play_sound("scroll", 1);

    } else if (pfrm.keyboard().down_transition<Key::left>()) {
        message_.reset();

        erase_selector();

        auto& updater = lines_[select_row_].updater_;

        draw_line(pfrm, select_row_, updater.update(pfrm, game, -1).c_str());

        updater.complete(pfrm, game, *this);

        pfrm.speaker().play_sound("scroll", 1);
    }

    if (not message_ and not message_anim_) {
        const auto& line = lines_[select_row_];
        const Float y_center = pfrm.screen().size().y / 2;
        const Float y_line = line.text_->coord().y * 8;
        const auto y_diff = (y_line - y_center) * 0.3f;

        y_offset_ = interpolate(Float(y_diff), y_offset_, delta * 0.00001f);

        pfrm.set_overlay_origin(0, y_offset_);
    }

    anim_timer_ += delta;
    if (anim_timer_ > milliseconds(75)) {
        anim_timer_ = 0;
        if (++anim_index_ > 1) {
            anim_index_ = 0;
        }

        auto [left, right] = [&]() -> Vec2<int> {
            switch (anim_index_) {
            default:
            case 0:
                return {147, 148};
            case 1:
                return {149, 150};
            }
        }();

        erase_selector();

        const auto& line = lines_[select_row_];

        pfrm.set_tile(
            Layer::overlay, line.cursor_begin_, line.text_->coord().y, left);
        pfrm.set_tile(
            Layer::overlay, line.cursor_end_, line.text_->coord().y, right);
    }

    return null_state();
}
