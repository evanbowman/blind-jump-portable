#include "script/lisp.hpp"
#include "state_impl.hpp"


void LanguageSelectionState::enter(Platform& pfrm,
                                   Game& game,
                                   State& prev_state)
{
    pfrm.enable_glyph_mode(true);

    auto languages = lisp::get_var("languages");
    const auto lang_count = lisp::length(languages);

    Vec2<u8> cursor;
    cursor.y = 3;

    const auto colors =
        Text::OptColors{{custom_color(0x193a77), custom_color(0xffffff)}};


    for (int i = 1; i < lang_count; ++i) {
        auto lang = lisp::get_list(languages, i);

        const auto double_size = lang->expect<lisp::Cons>()
                                     .cdr()
                                     ->expect<lisp::Cons>()
                                     .car()
                                     ->expect<lisp::Integer>()
                                     .value_ == 2;

        auto lang_name = locale_localized_language_name(pfrm, i);

        cursor.x = centered_text_margins(
            pfrm, utf8::len(lang_name->c_str()) * (double_size ? 2 : 1));

        FontConfiguration font_conf;
        font_conf.double_size_ = double_size;

        languages_.emplace_back(pfrm, cursor, font_conf);

        languages_.back().assign(lang_name->c_str(), colors);

        if (double_size) {
            cursor.y += 3;
        } else {
            cursor.y += 2;
        }
    }
}


void LanguageSelectionState::exit(Platform& pfrm, Game& game, State& next_state)
{
    languages_.clear();

    pfrm.fill_overlay(0);
}


void draw_cursor_image(Platform& pfrm, Text* target, int tile1, int tile2)
{
    const bool bigfont = target->config().double_size_;

    const auto pos = target->coord();
    pfrm.set_tile(Layer::overlay, pos.x - 2, pos.y + (bigfont ? 1 : 0), tile1);

    pfrm.set_tile(Layer::overlay,
                  pos.x + (target->len() * (bigfont ? 2 : 1)) + 1,
                  pos.y + (bigfont ? 1 : 0),
                  tile2);
}


StatePtr
LanguageSelectionState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    auto [left, right] = [&]() -> Vec2<int> {
        switch (anim_index_) {
        default:
        case 0:
            return {364, 365};
        case 1:
            return {366, 367};
        }
    }();

    cursor_anim_timer_ += delta;

    if (cursor_anim_timer_ > milliseconds(100)) {
        cursor_anim_timer_ = 0;
        anim_index_ = not anim_index_;
    }


    const auto& line = languages_[cursor_loc_];
    const auto y_center = pfrm.screen().size().y / 2;
    const Float y_line = line.coord().y * 8;
    const auto y_diff = (y_line - y_center) * 0.4f;

    y_offset_ = interpolate(Float(y_diff), y_offset_, delta * 0.00001f);

    pfrm.set_overlay_origin(0, y_offset_);


    draw_cursor_image(pfrm, &languages_[cursor_loc_], left, right);

    if (pfrm.keyboard().down_transition<Key::action_1>()) {
        pfrm.speaker().play_sound("select", 1);

        locale_set_language(cursor_loc_ + 1);

        game.persistent_data().settings_.language_.set(cursor_loc_ + 1);

        game.persistent_data().settings_.initial_lang_selected_ = true;

        return state_pool().create<TitleScreenState>();
    }


    if (pfrm.keyboard().down_transition<Key::down>()) {
        if (cursor_loc_ < (int)languages_.size() - 1) {
            draw_cursor_image(pfrm, &languages_[cursor_loc_], 0, 0);
            cursor_loc_ += 1;
            pfrm.speaker().play_sound("scroll", 1);
        }
    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        if (cursor_loc_ > 0) {
            draw_cursor_image(pfrm, &languages_[cursor_loc_], 0, 0);
            cursor_loc_ -= 1;
            pfrm.speaker().play_sound("scroll", 1);
        }
    }

    return null_state();
}
