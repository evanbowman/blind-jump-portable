#include "state_impl.hpp"


void GoodbyeState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.speaker().stop_music();

    pfrm.fill_overlay(112);

    if (auto tm = pfrm.system_clock().now()) {
        game.persistent_data().timestamp_ = *tm;
    }

    game.persistent_data().clean_ = false;
    pfrm.write_save_data(
        &game.persistent_data(), sizeof game.persistent_data(), 0);

    rng::critical_state = game.persistent_data().seed_.get();

    const bool bigfont = locale_requires_doublesize_font();

    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;

    const auto s_tiles = calc_screen_tiles(pfrm);
    text_.emplace(
        pfrm, OverlayCoord{1, u8(s_tiles.y - (bigfont ? 3 : 2))}, font_conf);
    text_->append(locale_string(pfrm, LocaleString::goodbye_text)->c_str());
}


void GoodbyeState::exit(Platform& pfrm, Game& game, State& next_state)
{
    text_.reset();
    pfrm.fill_overlay(0);
    game.camera() = {};
    game.camera().update(
        pfrm,
        Settings::CameraMode::fixed,
        milliseconds(100),
        {(float)pfrm.screen().size().x / 2, (float)pfrm.screen().size().y / 2});
    pfrm.set_overlay_origin(0, 0);
    game.enemies().clear();
    game.effects().clear();
    game.details().clear();
}


StatePtr GoodbyeState::update(Platform& pfrm, Game&, Microseconds delta)
{
    wait_timer_ += delta;

    static const auto hold_time = seconds(3);

    if (wait_timer_ > hold_time) {
        constexpr auto fade_duration = milliseconds(1500);
        if (wait_timer_ - hold_time > fade_duration) {
            return state_pool().create<TitleScreenState>();
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
