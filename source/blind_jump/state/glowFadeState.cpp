#include "state_impl.hpp"


StatePtr GlowFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    constexpr auto fade_duration = milliseconds(950);

    timer_ += delta;
    if (timer_ >
        milliseconds(interpolate(50, 140, Float(counter_) / fade_duration))) {
        // FIXME: Sprite scaling is broken on the desktop version of the game.
        // #ifdef __GBA__
        //         const int spread = 30;// interpolate(70, 20, Float(counter_) / fade_duration);
        //         game.effects().spawn<Particle>(game.player().get_position(),
        //                                        std::nullopt,
        //                                        0.0001038f,
        //                                        true,
        //                                        seconds(1),
        //                                        0,
        //                                        Float(270 - spread / 2) + rng::choice(spread, rng::utility_state));
        // #endif // __GBA__
        timer_ = 0;
    }

    counter_ += delta;

    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f, color_);
        pfrm.screen().pixelate(0, false);
        game.player().set_visible(false);

        // Just doing this because screen fades are expensive, and at this
        // point, we can no longer see these entities anyway, so let's get rid
        // of them.
        game.enemies().clear();
        game.details().clear();
        game.effects().clear();

        return state_pool().create<FadeOutState>(game, color_);
    } else {
        const auto amount = smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, color_);
        if (amount > 0.25f) {
            pfrm.screen().pixelate((amount - 0.25f) * 80, false);
        }
        return null_state();
    }
}
