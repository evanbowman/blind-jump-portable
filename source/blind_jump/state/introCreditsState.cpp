#include "state_impl.hpp"


void IntroCreditsState::center(Platform& pfrm)
{
    // Because the overlay uses 8x8 tiles, to truely center something, you
    // sometimes need to translate the whole layer.
    if (text_ and text_->len() % 2 == 0) {
        pfrm.set_overlay_origin(-4, 0);
    }
}


void IntroCreditsState::show_version(Platform& pfrm, Game& game)
{
    StringBuffer<31> version_str = "v";
    char str[10];

    auto push_vn = [&](int num) {
        locale_num2str(num, str, 10);
        version_str += str;
        version_str += ".";
    };

    push_vn(PROGRAM_MAJOR_VERSION);
    push_vn(PROGRAM_MINOR_VERSION);
    push_vn(PROGRAM_SUBMINOR_VERSION);
    push_vn(PROGRAM_VERSION_REVISION);

    const auto margin = centered_text_margins(pfrm, version_str.length());

    const auto screen_tiles = calc_screen_tiles(pfrm);

    version_.emplace(pfrm, OverlayCoord{u8(margin), u8(screen_tiles.y - 2)});
    version_->assign(
        version_str.c_str(),
        FontColors{ColorConstant::med_blue_gray, ColorConstant::rich_black});
}


void IntroCreditsState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.enable_glyph_mode(true);

    game.scavenger().reset(); // Bugfix

    auto pos = (pfrm.screen().size() / u32(8)).cast<u8>();

    const bool bigfont = locale_requires_doublesize_font();

    const char* creator_str = "Evan Bowman";

    // Center horizontally, and place text vertically in top third of screen
    const auto len =
        str_len(creator_str) + utf8::len(str_->c_str()) * (bigfont ? 2 : 1);
    pos.x -= len + 1;
    pos.x /= 2;
    pos.y *= 0.35f;

    if (len % 2 == 0) {
        pfrm.set_overlay_origin(-4, 0);
    }

    creator_.emplace(pfrm, creator_str, pos);

    pos.x += str_len(creator_str);

    if (bigfont) {
        pos.y -= 1;
    }

    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;

    text_.emplace(pfrm, str_->c_str(), pos, font_conf);

    // The translator for the chinese edition told me that I should put a "not
    // for commercial use" logo in the game somewhere, to discourage piracy.
    if (locale_language_name(locale_get_language()) == "chinese") {

        pos.y += 6;
        pos.x = 9;

        pfrm.set_tile(Layer::overlay, pos.x - 2, pos.y, 377);
        pfrm.set_tile(Layer::overlay, pos.x - 1, pos.y, 378);

        translator_.emplace(pfrm, pos);
        translator_->assign(
            locale_string(pfrm, LocaleString::translator_name)->c_str());

        pos.y += 4;
        pos.x += 2;
        pfrm.set_tile(Layer::overlay, pos.x + 1, pos.y, 368);
        pfrm.set_tile(Layer::overlay, pos.x + 2, pos.y, 369);
        pfrm.set_tile(Layer::overlay, pos.x + 3, pos.y, 370);
        pfrm.set_tile(Layer::overlay, pos.x + 4, pos.y, 371);
        pfrm.set_tile(Layer::overlay, pos.x + 5, pos.y, 372);

    } else {
        pos.y += 8;
        pos.x = 1;
        translator_.emplace(pfrm, pos);
        translator_->assign(
            locale_string(pfrm, LocaleString::translator_name)->c_str(),
            FontColors{ColorConstant::med_blue_gray,
                       ColorConstant::rich_black});
    }

    center(pfrm);
    // pfrm.screen().fade(0.f);
    pfrm.screen().fade(1.f);

    if (game.level() == 0) {
        pfrm.speaker().play_music("rocketlaunch", 0);
    }
}


void IntroCreditsState::exit(Platform& pfrm, Game& game, State&)
{
    text_.reset();
    creator_.reset();
    version_.reset();
    translator_.reset();
    pfrm.set_overlay_origin(0, 0);

    const auto st = calc_screen_tiles(pfrm);
    pfrm.set_tile(Layer::overlay, st.x / 2 - 1, st.y - 3, 0);
    pfrm.set_tile(Layer::overlay, st.x / 2, st.y - 3, 0);
}


StatePtr IntroCreditsState::next_state(Platform& pfrm, Game& game)
{
    if (pfrm.keyboard().pressed<Key::start>()) {
        return state_pool().create<EndingCreditsState>();
    }

    return state_pool().create<LaunchCutsceneState>();
}


static constexpr auto show_text_time = seconds(2) + milliseconds(900);
static constexpr auto wait_time = milliseconds(167) + seconds(2);


Microseconds IntroCreditsState::music_offset()
{
    return show_text_time + wait_time;
}


StatePtr
IntroCreditsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (pfrm.keyboard()
            .down_transition<Key::left,
                             Key::right,
                             Key::up,
                             Key::down,
                             Key::start,
                             Key::select,
                             Key::alt_1,
                             Key::alt_2>()) {

        show_version(pfrm, game);
    }

    center(pfrm);

    timer_ += delta;

    const auto skip = pfrm.keyboard().down_transition(game.action2_key());

    if (text_) {
        if (timer_ > show_text_time or skip) {
            text_.reset();
            creator_.reset();
            version_.reset();
            translator_.reset();
            pfrm.fill_overlay(0);
            timer_ = 0;

            if (skip) {
                if (game.level() == 0) {
                    pfrm.speaker().play_music("rocketlaunch", music_offset());
                }

                return next_state(pfrm, game);
            }
        }

    } else {
        if (timer_ > wait_time) {
            return next_state(pfrm, game);
        }
    }

    return null_state();
}
