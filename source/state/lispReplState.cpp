#include "state_impl.hpp"
#include "script/lisp.hpp"


// Inspired by the dvorak keyboard layout, redesigned for use with a gameboy
// dpad. Optimized for the smallest horizontal _and_ vertical travel between key
// presses. I've never understood why some devices, like the playstation or the
// kindle fire, don't at least give you the option of a keyboard better
// optimized for low travel when using a joystick or dpad.
static const char* keyboard[7][6] = {
    {"z", "y", "g", "f", "v", "q"},
    {"m", "b", "i", "d", "l", "j"},
    {"w", "a", "o", "e", "u", "k"},
    {"p", "h", "t", "n", "s", "r"},
    {"x", "c", "(", ")", "-", "*"},
    {" ", " ", "0", "1", "2", "3"},
    {"4", "5", "6", "7", "8", "9"}
};


void LispReplState::repaint_entry(Platform& pfrm)
{
    const auto screen_tiles = calc_screen_tiles(pfrm);

    entry_->assign(":", Text::OptColors{{ColorConstant::med_blue_gray,
                                         ColorConstant::rich_black}});

    for (int i = 1; i < 32; ++i) {
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 1, 112);
    }

    auto colors = [this]() -> Text::OptColors {
        switch (display_mode_) {
        default:
        case DisplayMode::entry:
            return {};

        case DisplayMode::show_result:
            return {{ColorConstant::med_blue_gray,
                     ColorConstant::rich_black}};
        }
    }();
    entry_->append(command_.c_str(), colors);

    keyboard_.clear();
    for (int i = 0; i < 7; ++i) {
        keyboard_.emplace_back(pfrm, OverlayCoord{1, u8(1 + i)});

        for (int j = 0; j < 6; ++j) {
            if (j == keyboard_cursor_.x and keyboard_cursor_.y == i) {
                const auto colors =
                    Text::OptColors{{
                        ColorConstant::rich_black,
                        ColorConstant::aerospace_orange}};
                keyboard_.back().append(::keyboard[i][j], colors);
            } else {
                keyboard_.back().append(::keyboard[i][j]);
            }
        }

    }
}


void LispReplState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    // pfrm.load_overlay_texture("repl");

    pfrm.screen().fade(0.34f);
    locale_set_language(LocaleLanguage::english);

    keyboard_cursor_ = {2, 4}; // For convenience, place cursor at left paren

    const auto screen_tiles = calc_screen_tiles(pfrm);

    entry_.emplace(pfrm, OverlayCoord{0, u8(screen_tiles.y - 1)});

    repaint_entry(pfrm);
}


void LispReplState::exit(Platform& pfrm, Game& game, State& next_state)
{
    locale_set_language(game.persistent_data().settings_.language_);

    entry_.reset();
}


StatePtr LispReplState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    switch (display_mode_) {
    case DisplayMode::entry:
        break;

    case DisplayMode::show_result:
        if (pfrm.keyboard().down_transition<Key::action_1,
                                            Key::action_2,
                                            Key::start,
                                            Key::left,
                                            Key::right,
                                            Key::up,
                                            Key::down>()) {

            display_mode_ = DisplayMode::entry;
            command_.clear();
            repaint_entry(pfrm);
            return null_state();
        }
        break;
    }
    if (pfrm.keyboard().down_transition(game.action1_key())) {
        if (command_.empty()) {
            return state_pool().create<PauseScreenState>(false);
        }
        command_.pop_back();
        repaint_entry(pfrm);
    }
    if (pfrm.keyboard().down_transition(game.action2_key())) {
        command_.push_back(keyboard[keyboard_cursor_.y][keyboard_cursor_.x][0]);
        repaint_entry(pfrm);
    }

    if (pfrm.keyboard().down_transition<Key::start>()) {

        lisp::eval(command_.c_str());

        command_ = format(lisp::get_op(0)).c_str();

        lisp::pop_op();

        display_mode_ = DisplayMode::show_result;

        repaint_entry(pfrm);
    }

    if (pfrm.keyboard().down_transition<Key::left>()) {
        if (keyboard_cursor_.x == 0) {
            keyboard_cursor_.x = 5;
        } else {
            keyboard_cursor_.x -= 1;
        }
        repaint_entry(pfrm);
    } else if (pfrm.keyboard().down_transition<Key::right>()) {
        if (keyboard_cursor_.x == 5) {
            keyboard_cursor_.x = 0;
        } else {
            keyboard_cursor_.x += 1;
        }
        repaint_entry(pfrm);
    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        if (keyboard_cursor_.y == 0) {
            keyboard_cursor_.y = 6;
        } else {
            keyboard_cursor_.y -= 1;
        }
        repaint_entry(pfrm);
    } else if (pfrm.keyboard().down_transition<Key::down>()) {
        if (keyboard_cursor_.y == 6) {
            keyboard_cursor_.y = 0;
        } else {
            keyboard_cursor_.y += 1;
        }
        repaint_entry(pfrm);
    }
    return null_state();
}
