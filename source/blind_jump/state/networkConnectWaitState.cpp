#include "state_impl.hpp"
#include "version.hpp"


void NetworkConnectWaitState::enter(Platform& pfrm,
                                    Game& game,
                                    State& prev_state)
{
    pfrm.load_overlay_texture("overlay_network_flattened");
    pfrm.fill_overlay(85);
    draw_image(pfrm, 85, 1, 4, 28, 13, Layer::overlay);
}


void NetworkConnectWaitState::exit(Platform& pfrm,
                                   Game& game,
                                   State& next_state)
{
    pfrm.fill_overlay(0);
    pfrm.screen().fade(1.f, ColorConstant::rich_black, {}, true, true);
    pfrm.load_overlay_texture("overlay");

    if (not dynamic_cast<MenuState*>(&next_state)) {

        if (auto os = dynamic_cast<OverworldState*>(&next_state)) {
            if (pfrm.network_peer().is_connected()) {
                push_notification(
                    pfrm,
                    os,
                    locale_string(pfrm, LocaleString::peer_connected)->c_str());

                // Not really necessary. But because the gameboy advance
                // handhelds will be in close proximity, nicer to have the music
                // aligned.
                auto& zone = zone_info(game.level());
                pfrm.speaker().play_music(zone.music_name_, 0);

                net_event::ProgramVersion vn;
                vn.info_.major_.set(PROGRAM_MAJOR_VERSION);
                vn.info_.minor_.set(PROGRAM_MINOR_VERSION);
                vn.info_.subminor_.set(PROGRAM_SUBMINOR_VERSION);
                vn.info_.revision_.set(PROGRAM_VERSION_REVISION);

                net_event::transmit(pfrm, vn);

            } else {
                push_notification(
                    pfrm,
                    os,
                    locale_string(pfrm, LocaleString::peer_connection_failed)
                        ->c_str());
            }
        }
    }
}


StatePtr
NetworkConnectWaitState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (not ready_) {
        ready_ = true;
        return null_state();
    }

    switch (pfrm.network_peer().interface()) {
    case Platform::NetworkPeer::serial_cable: {

        const char* str = " Waiting for Peer...";

        const auto margin = centered_text_margins(pfrm, utf8::len(str));

        Text t(pfrm, {static_cast<u8>(margin), 2});

        t.assign(str);
        pfrm.network_peer().listen();
        return state_pool().create<ActiveState>();
    }

    case Platform::NetworkPeer::internet: {
        // TODO: display some more interesting graphics, allow user to enter ip
        // address, display our ip address if we're the host.
        if (pfrm.keyboard().down_transition(game.action1_key())) {
            pfrm.network_peer().connect("127.0.0.1");
            return state_pool().create<PauseScreenState>(false);
        } else if (pfrm.keyboard().down_transition(game.action2_key())) {
            pfrm.network_peer().listen();

            net_event::SyncSeed s;
            s.random_state_.set(rng::critical_state);
            net_event::transmit(pfrm, s);

            return state_pool().create<PauseScreenState>(false);
        }
        break;
    }
    }

    return null_state();
}
