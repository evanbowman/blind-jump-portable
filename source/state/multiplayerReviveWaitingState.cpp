#include "state_impl.hpp"


void MultiplayerReviveWaitingState::receive(const net_event::PlayerEnteredGate&,
                                            Platform& pfrm,
                                            Game& game)
{
    // Ok, so at this point, the other player entered the warp gate and left us
    // behind. No one is coming to help, let's disconnect from the multiplayer
    // game ;(
    pfrm.network_peer().disconnect();
}


StatePtr MultiplayerReviveWaitingState::update(Platform& pfrm,
                                               Game& game,
                                               Microseconds delta)
{
    OverworldState::update(pfrm, game, delta);

    if (not pfrm.network_peer().is_connected() or
        not game.peer()) {
        if (pfrm.network_peer().is_connected()) {
            pfrm.network_peer().disconnect();
        }
        return state_pool().create<DeathFadeState>(game);
    }

    // The overworld state has a lot of useful stuff in it, but we want to use
    // our own camera, to track the peer player.
    camera_.update(pfrm,
                   game.persistent_data().settings_.camera_mode_,
                   delta,
                   game.peer()->get_position());

    return null_state();
}
