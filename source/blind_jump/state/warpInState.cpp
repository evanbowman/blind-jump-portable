#include "state_impl.hpp"


StatePtr WarpInState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    if (counter_ > milliseconds(200) and not shook_) {
        game.camera().shake();
        shook_ = true;
    }

    constexpr auto fade_duration = milliseconds(950);
    if (counter_ > fade_duration) {
        shook_ = false;
        pfrm.screen().fade(0.f, current_zone(game).energy_glow_color_);
        pfrm.screen().pixelate(0, false);

        if (game.peer()) {
            game.peer()->warping() = false;
        }

        if (is_boss_level(game.level())) {
            game.on_timeout(
                pfrm, milliseconds(100), [](Platform& pfrm, Game& game) {
                    push_notification(
                        pfrm,
                        game.state(),
                        locale_string(pfrm, LocaleString::power_surge_detected)
                            ->c_str());
                });
        }


        return state_pool().create<ActiveState>();
    } else {
        const auto amount = 1.f - smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, current_zone(game).energy_glow_color_);
        if (amount > 0.5f) {
            pfrm.screen().pixelate((amount - 0.5f) * 60, false);
        }
        return null_state();
    }
}
