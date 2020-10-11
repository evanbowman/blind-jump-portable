#include "state_impl.hpp"


void DialogState::display_time_remaining(Platform&, Game&)
{
}


void DialogState::enter(Platform& pfrm, Game& game, State& prev_state)
{
}


void DialogState::exit(Platform& pfrm, Game& game, State& next_state)
{
}


StatePtr DialogState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    OverworldState::update(pfrm, game, delta);

    return null_state();
}
