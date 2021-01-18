#include "state_impl.hpp"


StatePtr
NetworkConnectSetupState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    timer_ += delta;

    constexpr auto fade_duration = milliseconds(950);
    if (timer_ > fade_duration) {
        pfrm.screen().fade(1.f, ColorConstant::indigo_tint);

        switch (pfrm.network_peer().interface()) {
        case Platform::NetworkPeer::serial_cable:
            return state_pool().create<NetworkConnectWaitState>();
            break;

        case Platform::NetworkPeer::internet: {
            // TODO: display some more interesting graphics, allow user to enter
            // ip address
            return state_pool().create<NetworkConnectWaitState>();
            break;
        }
        }

    } else {
        const auto amount = smoothstep(0.f, fade_duration, timer_);
        pfrm.screen().fade(
            amount, ColorConstant::indigo_tint, ColorConstant::rich_black);
        return null_state();
    }

    return null_state();
}
