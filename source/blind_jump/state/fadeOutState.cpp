#include "state_impl.hpp"


StatePtr FadeOutState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(670);
    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f);

        return state_pool().create<NewLevelIdleState>();

    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           color_);

        return null_state();
    }
}
