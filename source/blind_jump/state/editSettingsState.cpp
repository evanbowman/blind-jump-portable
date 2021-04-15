#include "script/lisp.hpp"
#include "state_impl.hpp"


EditSettingsState::EditSettingsState(DeferredState exit_state)
    : exit_state_(exit_state), lines_{{{language_line_updater_},
                                       {swap_action_keys_line_updater_},
                                       {strafe_mode_line_updater_},
                                       {camera_mode_line_updater_},
                                       {difficulty_line_updater_},
                                       {contrast_line_updater_},
                                       {night_mode_line_updater_},
                                       {speedrun_clock_line_updater_},
                                       {rumble_enabled_line_updater_}}}
{
}


void EditSettingsState::draw_line(Platform& pfrm, int row, const char* value)
{
    auto str = locale_string(pfrm, strings[row]);

    const bool bigfont = locale_requires_doublesize_font();

    const int value_len = utf8::len(value) * (bigfont ? 2 : 1);
    const int field_len = utf8::len(str->c_str()) * (bigfont ? 2 : 1);

    auto margin = centered_text_margins(pfrm, value_len + field_len + 2);

    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;

    lines_[row].text_.emplace(
        pfrm,
        OverlayCoord{(u8)margin, u8(4 + row * (bigfont ? 3 : 2))},
        font_conf);

    lines_[row].text_->append(str->c_str());
    if (bigfont) {
        lines_[row].text_->append(" ");
    } else {
        lines_[row].text_->append("  ");
    }
    lines_[row].text_->append(value);

    lines_[row].cursor_begin_ = margin + field_len;
    lines_[row].cursor_end_ = margin + field_len + 2 + value_len + 1;

    if (bigfont) {
        lines_[row].cursor_end_ = margin + field_len + 2 + value_len + 1;
    }
}


void EditSettingsState::refresh(Platform& pfrm, Game& game)
{
    const bool bigfont = locale_requires_doublesize_font();

    for (auto& line : lines_) {
        line.text_.reset();
    }

    pfrm.fill_overlay(0);

    if (bigfont) {
        for (u32 i = 0; i < lines_.size(); ++i) { // FIXME!
            draw_line(
                pfrm, i, lines_[i].updater_.update(pfrm, game, 0).c_str());
        }
    } else {
        for (u32 i = 0; i < lines_.size(); ++i) {
            draw_line(
                pfrm, i, lines_[i].updater_.update(pfrm, game, 0).c_str());
        }
    }
}


void EditSettingsState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.enable_expanded_glyph_mode(true);

    refresh(pfrm, game);
}


void EditSettingsState::exit(Platform& pfrm, Game& game, State& next_state)
{
    // for (auto& l : lines_) {
    //     l.text_.reset();
    // }

    // pfrm.fill_overlay(0);

    // pfrm.set_overlay_origin(0, 0);
}


StatePtr
EditSettingsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition(game.action2_key())) {
        pfrm.speaker().play_sound("select", 1);
        exit_ = true;
        pfrm.enable_expanded_glyph_mode(false);

        for (auto& l : lines_) {
            l.text_.reset();
        }

        pfrm.fill_overlay(0);

        pfrm.set_overlay_origin(0, 0);

        return null_state();
    }

    if (exit_) {
        pfrm.load_overlay_texture("overlay");

        return exit_state_();
    }

    const bool bigfont = locale_requires_doublesize_font();

    auto erase_selector = [&] {
        for (u32 i = 0; i < lines_.size(); ++i) {
            const auto& line = lines_[i];
            pfrm.set_tile(Layer::overlay,
                          line.cursor_begin_,
                          line.text_->coord().y + 1,
                          0);
            pfrm.set_tile(
                Layer::overlay, line.cursor_end_, line.text_->coord().y + 1, 0);
            pfrm.set_tile(
                Layer::overlay, line.cursor_begin_, line.text_->coord().y, 0);
            pfrm.set_tile(
                Layer::overlay, line.cursor_end_, line.text_->coord().y, 0);
        }
    };

    if (pfrm.keyboard().down_transition<Key::down>()) {
        // if (not locale_requires_doublesize_font()) { // FIXME
        if (select_row_ < static_cast<int>(lines_.size() - 1)) {
            select_row_ += 1;
            pfrm.speaker().play_sound("scroll", 1);
        }
        // }
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

    if (bigfont) {
        const auto& line = lines_[select_row_];
        const Float y_center = pfrm.screen().size().y / 2 - 48;
        const Float y_line = line.text_->coord().y * 8;
        const auto y_diff = (y_line - y_center) * 0.6f;

        y_offset_ = interpolate(Float(y_diff), y_offset_, delta * 0.00001f);

        pfrm.set_overlay_origin(0, y_offset_);

    } else {
        const auto& line = lines_[select_row_];
        const Float y_center = pfrm.screen().size().y / 2;
        const Float y_line = line.text_->coord().y * 8;
        const auto y_diff = (y_line - y_center) * 0.4f;

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
                return {373, 374};
            case 1:
                return {375, 376};
            }
        }();

        erase_selector();

        const auto& line = lines_[select_row_];

        pfrm.set_tile(Layer::overlay,
                      line.cursor_begin_,
                      line.text_->coord().y + (bigfont ? 1 : 0),
                      left);
        pfrm.set_tile(Layer::overlay,
                      line.cursor_end_,
                      line.text_->coord().y + (bigfont ? 1 : 0),
                      right);
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
