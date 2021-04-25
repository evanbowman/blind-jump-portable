#include "state_impl.hpp"


void DeathFadeState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);
}


StatePtr DeathFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = seconds(2);
    if (counter_ > fade_duration) {
        pfrm.screen().pixelate(0);

        auto screen_tiles = calc_screen_tiles(pfrm);

        const auto image_width = 18;

        int image_y = 3;

        StringBuffer<48> death_text_overlay("death_text_");
        death_text_overlay += locale_language_name(locale_get_language());

        if (not pfrm.load_overlay_texture(death_text_overlay.c_str())) {
            pfrm.load_overlay_texture("death_text_english");
        }

        if (locale_language_name(locale_get_language()) == "russian") {
            // FIXME: go back into the other image files, and change the text,
            // so that, all of the death text images are the same size. Because
            // I'm lazy, I just added a special case for Russian text. I'll fix
            // the other image files later.
            const auto image_width = 20;
            draw_image(pfrm,
                       444,
                       (screen_tiles.x - image_width) / 2,
                       image_y,
                       image_width,
                       3,
                       Layer::overlay);
        } else {
            draw_image(pfrm,
                       450,
                       (screen_tiles.x - image_width) / 2,
                       image_y,
                       image_width,
                       3,
                       Layer::overlay);
        }


        if (pfrm.network_peer().is_connected()) {
            if (game.peer()) {
                game.peer().reset();
            }
            safe_disconnect(pfrm);
        }

        return state_pool().create<DeathContinueState>();
    } else {
        const auto amount = smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, current_zone(game).injury_glow_color_);
        pfrm.screen().pixelate(amount * 100);
        return null_state();
    }
}
