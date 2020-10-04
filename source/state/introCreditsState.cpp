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
    version_->assign(version_str.c_str(),
                     FontColors{ColorConstant::med_blue_gray,
                                ColorConstant::rich_black});
}


void IntroCreditsState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.enable_glyph_mode(true);

    auto pos = (pfrm.screen().size() / u32(8)).cast<u8>();

    // Center horizontally, and place text vertically in top third of screen
    const auto len = utf8::len(str_.obj_->c_str());
    pos.x -= len + 1;
    pos.x /= 2;
    pos.y *= 0.35f;

    if (len % 2 == 0) {
        pfrm.set_overlay_origin(-4, 0);
    }
    text_.emplace(pfrm, str_.obj_->c_str(), pos);

    center(pfrm);
    // pfrm.screen().fade(0.f);
    pfrm.screen().fade(1.f);
}


void IntroCreditsState::exit(Platform& pfrm, Game& game, State&)
{
    text_.reset();
    version_.reset();
    pfrm.set_overlay_origin(0, 0);
}


StatePtr IntroCreditsState::next_state(Platform& pfrm, Game& game)
{
    if (pfrm.keyboard().pressed<Key::start>()) {
        return state_pool().create<EndingCreditsState>();
    }

    if (game.level() == 0) {
        return state_pool().create<LaunchCutsceneState>();
    } else {
        return state_pool().create<NewLevelState>(game.level());
    }
}


StatePtr
IntroCreditsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (pfrm.keyboard().down_transition<Key::left,
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
        if (timer_ > seconds(2) + milliseconds(900) or skip) {
            text_.reset();
            version_.reset();
            timer_ = 0;

            if (skip) {
                return next_state(pfrm, game);
            }
        }

    } else {
        if (timer_ > milliseconds(167) + seconds(2)) {
            return next_state(pfrm, game);
        }
    }

    return null_state();
}
