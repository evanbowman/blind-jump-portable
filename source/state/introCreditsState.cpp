#include "state_impl.hpp"


void IntroCreditsState::center(Platform& pfrm)
{
    // Because the overlay uses 8x8 tiles, to truely center something, you
    // sometimes need to translate the whole layer.
    if (text_ and text_->len() % 2 == 0) {
        pfrm.set_overlay_origin(-4, 0);
    }
}


void IntroCreditsState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.enable_glyph_mode(true);

    auto pos = (pfrm.screen().size() / u32(8)).cast<u8>();

    // Center horizontally, and place text vertically in top third of screen
    const auto len = utf8::len(str_);
    pos.x -= len + 1;
    pos.x /= 2;
    pos.y *= 0.35f;

    if (len % 2 == 0) {
        pfrm.set_overlay_origin(-4, 0);
    }
    text_.emplace(pfrm, str_, pos);

    center(pfrm);

    pfrm.screen().fade(1.f);
}


void IntroCreditsState::exit(Platform& pfrm, Game& game, State&)
{
    text_.reset();
    pfrm.set_overlay_origin(0, 0);
}


StatePtr IntroLegalMessage::next_state(Platform& pfrm, Game& game)
{
    return state_pool().create<IntroCreditsState>(
        locale_string(LocaleString::intro_text_2));
}


StatePtr IntroCreditsState::next_state(Platform& pfrm, Game& game)
{
    // backdoor for debugging purposes.
    if (pfrm.keyboard().all_pressed<Key::alt_1, Key::alt_2, Key::start>()) {
        return state_pool().create<CommandCodeState>();
    }

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
    center(pfrm);

    timer_ += delta;

    const auto skip = pfrm.keyboard().down_transition(game.action2_key());

    if (text_) {
        if (timer_ > seconds(2) + milliseconds(500) or skip) {
            text_.reset();
            timer_ = 0;

            if (skip) {
                return next_state(pfrm, game);
            }
        }

    } else {
        if (timer_ > milliseconds(167) + seconds(1)) {
            return next_state(pfrm, game);
        }
    }

    return null_state();
}
