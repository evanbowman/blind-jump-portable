#include "script/lisp.hpp"
#include "state_impl.hpp"


EditSettingsState::EditSettingsState(DeferredState exit_state)
    : exit_state_(exit_state), lines_{{{swap_action_keys_line_updater_},
                                       {strafe_mode_line_updater_},
                                       {camera_mode_line_updater_},
                                       {difficulty_line_updater_},
                                       {language_line_updater_},
                                       {contrast_line_updater_},
                                       {night_mode_line_updater_},
                                       {show_stats_line_updater_},
                                       {speedrun_clock_line_updater_},
                                       {rumble_enabled_line_updater_}}}
{
}


void EditSettingsState::draw_line(Platform& pfrm, int row, const char* value)
{
    auto str = locale_string(pfrm, strings[row]);

    const int value_len = utf8::len(value);
    const int field_len = utf8::len(str->c_str());

    const auto margin = centered_text_margins(pfrm, value_len + field_len + 2);

    lines_[row].text_.emplace(pfrm, OverlayCoord{0, u8(4 + row * 2)});

    left_text_margin(*lines_[row].text_, margin);
    lines_[row].text_->append(str->c_str());
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
        if (select_row_ < static_cast<int>(lines_.size() - 1)) {
            select_row_ += 1;
            pfrm.speaker().play_sound("scroll", 1);
        }
    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        if (select_row_ > 0) {
            select_row_ -= 1;
            pfrm.speaker().play_sound("scroll", 1);
        }
    } else if (pfrm.keyboard().down_transition<Key::right>()) {

        erase_selector();

        auto& updater = lines_[select_row_].updater_;

        draw_line(pfrm, select_row_, updater.update(pfrm, game, 1).c_str());

        updater.complete(pfrm, game, *this);

        pfrm.speaker().play_sound("scroll", 1);

    } else if (pfrm.keyboard().down_transition<Key::left>()) {

        erase_selector();

        auto& updater = lines_[select_row_].updater_;

        draw_line(pfrm, select_row_, updater.update(pfrm, game, -1).c_str());

        updater.complete(pfrm, game, *this);

        pfrm.speaker().play_sound("scroll", 1);
    }

    const auto& line = lines_[select_row_];
    const Float y_center = pfrm.screen().size().y / 2;
    const Float y_line = line.text_->coord().y * 8;
    const auto y_diff = (y_line - y_center) * 0.4f;

    y_offset_ = interpolate(Float(y_diff), y_offset_, delta * 0.00001f);

    pfrm.set_overlay_origin(0, y_offset_);

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


EditSettingsState::LineUpdater::Result
EditSettingsState::LanguageLineUpdater::update(Platform& pfrm,
                                               Game& game,
                                               int dir)
{


    auto& language = game.persistent_data().settings_.language_;
    int l = static_cast<int>(language.get());

    const auto lang_count = lisp::length(lisp::get_var("languages"));

    if (dir > 0) {
        l += 1;
        l %= static_cast<int>(lang_count);
        if (l == 0) {
            l = 1;
        }
    } else if (dir < 0) {
        if (l > 1) {
            l -= 1;
        } else if (l == 1) {
            l = static_cast<int>(lang_count) - 1;
        }
    }

    language.set(l);
    locale_set_language(language.get());

    return locale_string(pfrm, LocaleString::language_name)->c_str();
}