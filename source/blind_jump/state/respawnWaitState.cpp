#include "state_impl.hpp"


void RespawnWaitState::enter(Platform& pfrm, Game& game, State&)
{
    // pfrm.speaker().play_sound("bell", 5);
}


StatePtr
RespawnWaitState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    if (counter_ > milliseconds(800)) {
        return state_pool().create<LaunchCutsceneState>();
    }

    return null_state();
}
