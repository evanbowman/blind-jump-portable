#include "localization.hpp"
#include "state_impl.hpp"


void DialogState::display_time_remaining(Platform&, Game&)
{
}


void DialogState::clear_textbox(Platform& pfrm)
{
    const auto st = calc_screen_tiles(pfrm);

    for (int x = 1; x < st.x - 1; ++x) {
        pfrm.set_tile(Layer::overlay, x, st.y - 5, 84);
        pfrm.set_tile(Layer::overlay, x, st.y - 4, 82);
        pfrm.set_tile(Layer::overlay, x, st.y - 3, 82);
        pfrm.set_tile(Layer::overlay, x, st.y - 2, 82);
        pfrm.set_tile(Layer::overlay, x, st.y - 1, 85);
    }

    pfrm.set_tile(Layer::overlay, 0, st.y - 4, 89);
    pfrm.set_tile(Layer::overlay, 0, st.y - 3, 89);
    pfrm.set_tile(Layer::overlay, 0, st.y - 2, 89);

    pfrm.set_tile(Layer::overlay, st.x - 1, st.y - 4, 88);
    pfrm.set_tile(Layer::overlay, st.x - 1, st.y - 3, 88);
    pfrm.set_tile(Layer::overlay, st.x - 1, st.y - 2, 88);

    pfrm.set_tile(Layer::overlay, 0, st.y - 5, 83);
    pfrm.set_tile(Layer::overlay, 0, st.y - 1, 90);
    pfrm.set_tile(Layer::overlay, st.x - 1, st.y - 5, 87);
    pfrm.set_tile(Layer::overlay, st.x - 1, st.y - 1, 86);

    text_state_.line_ = 0;
    text_state_.pos_ = 0;
}


void DialogState::init_text(Platform& pfrm, LocaleString str)
{
    clear_textbox(pfrm);

    text_state_.text_ = locale_string(pfrm, str);
    text_state_.current_word_remaining_ = 0;
    text_state_.current_word_ = (*text_state_.text_)->c_str();
    text_state_.timer_ = 0;
    text_state_.line_ = 0;
    text_state_.pos_ = 0;
}


Platform::TextureCpMapper locale_texture_map();


bool DialogState::advance_text(Platform& pfrm,
                               Game& game,
                               Microseconds delta,
                               bool sfx)
{
    if (asian_language_) {
        return advance_asian_text(pfrm, game, delta, sfx);
    }

    const auto delay = milliseconds(80);

    text_state_.timer_ += delta;

    const auto st = calc_screen_tiles(pfrm);

    if (text_state_.timer_ > delay) {
        text_state_.timer_ = 0;

        if (sfx) {
            pfrm.speaker().play_sound("msg", 5);
        }

        if (text_state_.current_word_remaining_ == 0) {
            while (*text_state_.current_word_ == ' ') {
                text_state_.current_word_++;
                if (text_state_.pos_ < st.x - 2) {
                    text_state_.pos_ += 1;
                }
            }
            bool done = false;
            utf8::scan(
                [&](const utf8::Codepoint& cp, const char*, int) {
                    if (done) {
                        return;
                    }
                    if (cp == ' ') {
                        done = true;
                    } else {
                        text_state_.current_word_remaining_++;
                    }
                },
                text_state_.current_word_,
                str_len(text_state_.current_word_));
        }

        // At this point, we know the length of the next space-delimited word in
        // the string. Now we can print stuff...

        const auto st = calc_screen_tiles(pfrm);
        static const auto margin_sum = 2;
        const auto text_box_width = st.x - margin_sum;
        const auto remaining = (text_box_width - text_state_.pos_) -
                               (text_state_.line_ == 0 ? 0 : 1);

        if (remaining < text_state_.current_word_remaining_) {
            if (text_state_.line_ == 0) {
                text_state_.line_++;
                text_state_.pos_ = 0;
                return true;
            } else {
                return false;
            }
        }

        int bytes_consumed = 0;
        const auto cp = utf8::getc(text_state_.current_word_, &bytes_consumed);

        const auto mapping_info = locale_texture_map()(cp);

        u16 t = 495; // bad glyph, FIXME: add a constant

        if (mapping_info) {
            t = pfrm.map_glyph(cp, *mapping_info);
        }

        const int y_offset = text_state_.line_ == 0 ? 4 : 2;
        const int x_offset = text_state_.pos_ + 1;

        pfrm.set_tile(Layer::overlay, x_offset, st.y - y_offset, t);

        text_state_.current_word_remaining_--;
        text_state_.current_word_ += bytes_consumed;
        text_state_.pos_++;

        if (*text_state_.current_word_ == '\0') {
            display_mode_ = DisplayMode::key_released_check2;
        }
    }

    return true;
}


void print_double_char(Platform& pfrm,
                       utf8::Codepoint c,
                       const OverlayCoord& coord,
                       const std::optional<FontColors>& colors = {});


bool DialogState::advance_asian_text(Platform& pfrm,
                                     Game& game,
                                     Microseconds delta,
                                     bool sfx)
{
    const auto delay = milliseconds(80);

    text_state_.timer_ += delta;

    if (text_state_.timer_ > delay) {
        text_state_.timer_ = 0;

        if (sfx) {
            pfrm.speaker().play_sound("msg", 5);
        }

        // At this point, we know the length of the next space-delimited word in
        // the string. Now we can print stuff...

        const auto st = calc_screen_tiles(pfrm);
        static const auto margin_sum = 2;
        const auto text_box_width = st.x - margin_sum;
        const auto remaining = (text_box_width - text_state_.pos_ * 2);

        if (remaining == 0) {
            return false;
        }

        int bytes_consumed = 0;
        const auto cp = utf8::getc(text_state_.current_word_, &bytes_consumed);

        const int x_offset = text_state_.pos_ * 2 + 1;

        print_double_char(pfrm, cp, {u8(x_offset), u8(st.y - 4)});

        text_state_.current_word_remaining_--;
        text_state_.current_word_ += bytes_consumed;
        text_state_.pos_++;

        if (*text_state_.current_word_ == '\0') {
            display_mode_ = DisplayMode::key_released_check2;
        }
    }

    return true;
}


void DialogState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);

    pfrm.load_overlay_texture("overlay_dialog");

    asian_language_ =
        (locale_language_name(locale_get_language()) == "chinese");


    init_text(pfrm, text_[0]); // TODO: implement chains of dialog messages,
                               // rather than just printing the first one in the
                               // list.
}


void DialogState::exit(Platform& pfrm, Game& game, State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);

    pfrm.load_overlay_texture("overlay");
}


StatePtr DialogState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    OverworldState::update(pfrm, game, delta);

    auto animate_moretext_icon = [&] {
        static const auto duration = milliseconds(500);
        text_state_.timer_ += delta;
        if (text_state_.timer_ > duration) {
            text_state_.timer_ = 0;
            const auto st = calc_screen_tiles(pfrm);
            if (pfrm.get_tile(Layer::overlay, st.x - 2, st.y - 2) == 91) {
                pfrm.set_tile(Layer::overlay, st.x - 2, st.y - 2, 92);
            } else {
                pfrm.set_tile(Layer::overlay, st.x - 2, st.y - 2, 91);
            }
        }
    };

    switch (display_mode_) {
    case DisplayMode::animate_in:
        display_mode_ = DisplayMode::busy;
        break;

    case DisplayMode::busy: {

        const bool text_busy = advance_text(pfrm, game, delta);

        if (not text_busy) {
            display_mode_ = DisplayMode::key_released_check1;
        } else {
            if (pfrm.keyboard().down_transition(game.action2_key()) or
                pfrm.keyboard().down_transition(game.action1_key())) {

                while (advance_text(pfrm, game, delta, false)) {
                    if (display_mode_ not_eq DisplayMode::busy) {
                        break;
                    }
                }

                if (display_mode_ == DisplayMode::busy) {
                    display_mode_ = DisplayMode::key_released_check1;
                }
            }
        }
    } break;

    case DisplayMode::wait: {
        animate_moretext_icon();

        if (pfrm.keyboard().down_transition(game.action2_key()) or
            pfrm.keyboard().down_transition(game.action1_key())) {

            text_state_.timer_ = 0;

            clear_textbox(pfrm);
            display_mode_ = DisplayMode::busy;
        }
        break;
    }

    case DisplayMode::key_released_check1:
        if (not pfrm.keyboard().pressed(game.action2_key()) and
            not pfrm.keyboard().pressed(game.action1_key())) {

            text_state_.timer_ = seconds(1);
            display_mode_ = DisplayMode::wait;
        }
        break;

    case DisplayMode::key_released_check2:
        if (not pfrm.keyboard().down_transition(game.action2_key()) and
            not pfrm.keyboard().down_transition(game.action1_key())) {

            text_state_.timer_ = seconds(1);
            display_mode_ = DisplayMode::done;
        }
        break;

    case DisplayMode::done:
        animate_moretext_icon();
        if (pfrm.keyboard().down_transition(game.action2_key()) or
            pfrm.keyboard().down_transition(game.action1_key())) {

            if (text_[1] not_eq LocaleString::empty) {
                ++text_;
                init_text(pfrm, *text_);
                display_mode_ = DisplayMode::animate_in;
            } else {
                display_mode_ = DisplayMode::animate_out;
            }
        }
        break;

    case DisplayMode::animate_out:
        display_mode_ = DisplayMode::clear;
        pfrm.fill_overlay(0);
        break;

    case DisplayMode::clear:
        return exit_state_();
    }

    return null_state();
}
