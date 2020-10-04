#include "state_impl.hpp"


void HealthAndSafetyWarningState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.load_overlay_texture("overlay_journal");

    pfrm.enable_glyph_mode(true);
    tv_.emplace(pfrm);

    pfrm.fill_overlay(82);

    auto screen_tiles = calc_screen_tiles(pfrm);

    text_.emplace(pfrm,
                  locale_string(pfrm, LocaleString::health_safety_notice)
                  .obj_->c_str(),
                  OverlayCoord{1, 1});
    tv_->assign(locale_string(pfrm, LocaleString::health_safety_text)
                .obj_->c_str(),
                {1, 4},
                OverlayCoord{u8(screen_tiles.x - 2), u8(screen_tiles.y - 4)});
}


void HealthAndSafetyWarningState::exit(Platform& pfrm, Game& game, State& next_state)
{
}


StatePtr HealthAndSafetyWarningState::update(Platform& pfrm,
                                             Game& game,
                                             Microseconds delta)
{
    switch (display_mode_) {
    case DisplayMode::wait:
        if (pfrm.keyboard().down_transition<Key::action_1,
                                            Key::action_2,
                                            Key::start,
                                            Key::select>()) {
            display_mode_ = DisplayMode::fade_out;
        }
        break;

    case DisplayMode::fade_out: {
        timer_ += delta;

        constexpr auto fade_duration = milliseconds(670);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(0.95f, ColorConstant::rich_black,
                               {},
                               true,
                               true);

            display_mode_ = DisplayMode::swap_texture;

        } else {
            pfrm.screen().fade(smoothstep(0.f, fade_duration, timer_),
                               ColorConstant::rich_black,
                               {},
                               true,
                               true);
        }
    } break;

    case DisplayMode::swap_texture:
        tv_.reset();
        text_.reset();
        pfrm.fill_overlay(0);
        pfrm.load_overlay_texture("overlay");
        display_mode_ = DisplayMode::exit;
        break;

    case DisplayMode::exit: {
        auto str = locale_string(pfrm, LocaleString::intro_text_2);
        return state_pool().create<IntroCreditsState>(std::move(str));
    } break;
    }

    return null_state();
}
