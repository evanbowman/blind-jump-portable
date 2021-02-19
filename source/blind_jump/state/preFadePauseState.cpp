#include "state_impl.hpp"


StatePtr
PreFadePauseState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.camera().set_speed(0.75f);

    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    timer_ += delta;
    if (timer_ > milliseconds(200)) {
        // FIXME: Sprite scaling is broken on the desktop version of the game.
        // #ifdef __GBA__
        //         game.effects().spawn<Particle>(game.player().get_position(),
        //                                        std::nullopt,
        //                                        0.0001038f,
        //                                        true,
        //                                        seconds(1),
        //                                        0,
        //                                        Float(220) + rng::choice<100>(rng::utility_state));
        // #endif // __GBA__
        timer_ = 0;
    }

#ifdef __PSP__
    return state_pool().create<GlowFadeState>(game, fade_color_);
    // FIXME: Due to the differing screen size, the code below does not work. I
    // really should use a timer, rather than waiting for the camera to reach a
    // certain spot...
#endif

    if (manhattan_length(pfrm.screen().get_view().get_center() +
                             pfrm.screen().get_view().get_size() / 2.f,
                         game.player().get_position()) < 18) {
        game.camera().set_speed(1.f);
        return state_pool().create<GlowFadeState>(game, fade_color_);
    } else {
        return null_state();
    }
}
