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

        const auto remaining =
            game.persistent_data().oxygen_remaining_.whole_seconds();

        auto oxygen_restore = [&] {
            switch (game.difficulty()) {
            case Settings::Difficulty::easy:
                return 70;

            case Settings::Difficulty::count:
            case Settings::Difficulty::normal:
                break;

            case Settings::Difficulty::survival:
            case Settings::Difficulty::hard:
                return 45;
            }
            return 55;
        }();

        if (is_boss_level(game.level() - 1)) {
            oxygen_restore *= 2;
        }

        const auto new_oxygen_val =
            std::min(60 * 6 + 30, remaining + oxygen_restore);

        game.persistent_data().oxygen_remaining_.reset(new_oxygen_val);

        if (game.level() < 3) {
            game.on_timeout(
                pfrm,
                milliseconds(100),
                [oxygen_restore](Platform& pfrm, Game& game) {
                    StringBuffer<31> msg;
                    msg += locale_string(pfrm, LocaleString::o2_restored_before)
                               ->c_str();

                    char buffer[10];
                    locale_num2str(oxygen_restore, buffer, 10);

                    msg += buffer;

                    msg += locale_string(pfrm, LocaleString::o2_restored_after)
                               ->c_str();

                    push_notification(pfrm, game.state(), msg.c_str());
                });
        }

        return state_pool().create<ActiveState>(game);
    } else {
        const auto amount = 1.f - smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, current_zone(game).energy_glow_color_);
        if (amount > 0.5f) {
            pfrm.screen().pixelate((amount - 0.5f) * 60, false);
        }
        return null_state();
    }
}
