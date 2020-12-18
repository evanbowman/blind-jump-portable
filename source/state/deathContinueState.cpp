#include "state_impl.hpp"


void DeathContinueState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    ScoreScreenState::enter(pfrm, game, prev_state);
}


void DeathContinueState::exit(Platform& pfrm, Game& game, State& next_state)
{
    ScoreScreenState::exit(pfrm, game, next_state);

    pfrm.fill_overlay(0);
}


StatePtr
DeathContinueState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    constexpr auto fade_duration = seconds(1);
    if (counter_ > fade_duration) {
        counter_ -= delta;
        pfrm.screen().fade(1.f,
                           ColorConstant::rich_black,
                           current_zone(game).injury_glow_color_);

        constexpr auto stats_time = milliseconds(500);

        if (counter2_ < stats_time) {
            const bool show_stats = counter2_ + delta > stats_time;

            counter2_ += delta;

            if (show_stats) {
                repaint_stats(pfrm, game);
            }
        } else {
            ScoreScreenState::update(pfrm, game, delta);
        }

        if (not locked() and not has_lines()) {
            if (pfrm.keyboard().pressed(game.action1_key()) or
                pfrm.keyboard().pressed(game.action2_key())) {

                game.score() = 0;
                game.player().revive(pfrm);

                return state_pool().create<RespawnWaitState>();
            }
        }
        return null_state();

    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           current_zone(game).injury_glow_color_);
        return null_state();
    }
}
