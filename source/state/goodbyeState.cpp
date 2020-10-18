#include "state_impl.hpp"


void GoodbyeState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.speaker().stop_music();

    pfrm.fill_overlay(112);

    if (auto tm = pfrm.system_clock().now()) {
        game.persistent_data().timestamp_ = *tm;
    }
    game.persistent_data().clean_ = false;
    pfrm.write_save_data(&game.persistent_data(),
                         sizeof game.persistent_data());

    const auto s_tiles = calc_screen_tiles(pfrm);
    text_.emplace(pfrm, OverlayCoord{1, u8(s_tiles.y - 2)});
    text_->append(locale_string(pfrm, LocaleString::goodbye_text)->c_str());
}


StatePtr GoodbyeState::update(Platform& pfrm, Game&, Microseconds delta)
{
    wait_timer_ += delta;

    static const auto hold_time = seconds(5);

    if (wait_timer_ > hold_time) {
        constexpr auto fade_duration = milliseconds(1500);
        if (wait_timer_ - hold_time > fade_duration) {
            pfrm.fatal();
        } else {
            const auto amount =
                smoothstep(0.f, fade_duration, wait_timer_ - hold_time);

            pfrm.screen().fade(
                amount, ColorConstant::rich_black, {}, true, true);

            return null_state();
        }
    }

    return null_state();
}
