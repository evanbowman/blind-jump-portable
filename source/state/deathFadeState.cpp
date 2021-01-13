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

        draw_image(pfrm,
                   450,
                   (screen_tiles.x - image_width) / 2,
                   3,
                   image_width,
                   3,
                   Layer::overlay);

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
