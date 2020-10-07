#include "state_impl.hpp"


StatePtr GlowFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(950);
    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f, color_);
        pfrm.screen().pixelate(0, false);
        game.player().set_visible(false);
        return state_pool().create<FadeOutState>(game, color_);
    } else {
        const auto amount = smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, color_);
        if (amount > 0.25f) {
            pfrm.screen().pixelate((amount - 0.25f) * 60, false);
        }
        return null_state();
    }
}
