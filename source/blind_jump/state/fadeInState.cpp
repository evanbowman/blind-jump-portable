#include "state_impl.hpp"


void FadeInState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);

    game.player().set_visible(game.level() == 0);
    game.camera().set_speed(0.75);

    if (game.peer()) {
        game.peer()->warping() = true;
    }
}


void FadeInState::exit(Platform& pfrm, Game& game, State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);

    game.camera().set_speed(1.f);
}


StatePtr FadeInState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(800);
    if (counter_ > fade_duration) {
        if (game.level() == 0) {
            return state_pool().create<ActiveState>();
        } else {
            pfrm.screen().fade(1.f, current_zone(game).energy_glow_color_);
            pfrm.sleep(2);
            game.player().set_visible(true);

            game.rumble(pfrm, milliseconds(250));

            return state_pool().create<WarpInState>(game);
        }
    } else {
        const auto amount = 1.f - smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount);
        return null_state();
    }
}
