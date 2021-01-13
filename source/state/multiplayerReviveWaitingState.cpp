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


void MultiplayerReviveWaitingState::enter(Platform& pfrm,
                                          Game& game,
                                          State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);

    camera_ = game.camera();
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

    Vec2<Float> camera_target;

    switch (display_mode_) {
    case DisplayMode::camera_pan: {
        timer_ += delta;
        static const auto pan_duration = seconds(2);

        camera_target = interpolate(game.peer()->get_position(),
                                    player_death_pos_,
                                    float(timer_) / pan_duration);

        if (timer_ > pan_duration) {
            timer_ = 0;
            display_mode_ = DisplayMode::track_peer;
        }
        break;
    }

    case DisplayMode::track_peer:
        // Safe access, because we checked contents of peer optional above.
        camera_target = game.peer()->get_position();
        break;
    }

    // The overworld state has a lot of useful stuff in it, but we want to use
    // our own camera, to track the peer player.
    camera_.update(pfrm,
                   game.persistent_data().settings_.camera_mode_,
                   delta,
                   camera_target);

    return null_state();
}
