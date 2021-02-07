#include "script/lisp.hpp"
#include "state_impl.hpp"


void RemoteReplState::enter(Platform& pfrm, Game& game, State& prev_state)
{
}


StatePtr RemoteReplState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    return null_state();
}
