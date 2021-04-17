#include "state_impl.hpp"


void NewLevelIdleState::receive(const net_event::NewLevelIdle&,
                                Platform& pfrm,
                                Game&)
{
    info(pfrm, "got new level idle msg");
    peer_ready_ = true;
}


void NewLevelIdleState::receive(const net_event::NewLevelSyncSeed& sync_seed,
                                Platform& pfrm,
                                Game& game)
{
    const auto peer_seed = sync_seed.random_state_.get();
    if (rng::critical_state == peer_seed) {

        if (matching_syncs_received_ == 0) {
            display_text(pfrm, LocaleString::level_transition_synchronizing);

            auto st = calc_screen_tiles(pfrm);

            loading_bar_.emplace(
                pfrm,
                6,
                OverlayCoord{u8(-1 + (st.x - 6) / 2), (u8)(st.y / 2 + 2)});
        }

        matching_syncs_received_ += 1;

        static const int required_matching_syncs = 10;

        if (loading_bar_) {
            loading_bar_->set_progress(Float(matching_syncs_received_) /
                                       required_matching_syncs);
        }

        if (matching_syncs_received_ == required_matching_syncs) {

            loading_bar_->set_progress(1.f);

            // We're ready, but what if, for some reason, the other peer has one
            // or more fewer matching syncs than we do? In that case, let's spam
            // our own seed just to be sure.
            for (int i = 0; i < required_matching_syncs; ++i) {
                net_event::NewLevelSyncSeed sync_seed;
                sync_seed.random_state_.set(rng::critical_state);
                sync_seed.difficulty_ = static_cast<u8>(game.difficulty());
                net_event::transmit(pfrm, sync_seed);
                pfrm.sleep(1);
            }

            ready_ = true;
        }
    } else {
        game.persistent_data().settings_.difficulty_ =
            static_cast<Settings::Difficulty>(sync_seed.difficulty_);

        rng::critical_state = peer_seed;
        matching_syncs_received_ = 0;
    }
}


void NewLevelIdleState::display_text(Platform& pfrm, LocaleString ls)
{
    const bool bigfont = locale_requires_doublesize_font();

    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;

    const auto str = locale_string(pfrm, ls);

    auto margin = centered_text_margins(pfrm, utf8::len(str->c_str()));

    if (bigfont) {
        margin -= utf8::len(str->c_str()) / 2;
    }

    auto screen_tiles = calc_screen_tiles(pfrm);

    text_.emplace(
        pfrm,
        OverlayCoord{(u8)margin, (u8)(screen_tiles.y / 2 - (bigfont ? 2 : 1))},
        font_conf);

    text_->assign(str->c_str());
}


void NewLevelIdleState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    if (pfrm.network_peer().is_connected()) {
        display_text(pfrm, LocaleString::level_transition_awaiting_peers);
    }
}


void NewLevelIdleState::exit(Platform& pfrm, Game& game, State& next_state)
{
    loading_bar_.reset();
    text_.reset();
}


StatePtr
NewLevelIdleState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    // We don't allow players to pause the game when multiplayer peers are
    // connected, because allowing pauses complicates the game logic, but we can
    // at least allow players to edit the settings while they're waiting in a
    // transporter for the next level.
    if (not peer_ready_ and pfrm.keyboard().down_transition<Key::start>()) {
        return state_pool().create<EditSettingsState>(
            make_deferred_state<NewLevelIdleState>());
    }

    // Synchronization procedure for seed values at level transition:
    //
    // Players transmit NewLevelIdle messages until both players are ready. Once
    // a device receives a NewLevelIdleMessage, it starts transmitting its
    // current random seed value. Upon receiving another device's random seed
    // value, a device resets its own random seed to the received value, if the
    // seed values do not match. After receiving N matching seed values, both
    // peers should advance to the next level.

    if (pfrm.network_peer().is_connected()) {
        timer_ += delta;
        if (timer_ > milliseconds(250)) {
            info(pfrm, "transmit new level idle msg");
            timer_ -= milliseconds(250);

            net_event::NewLevelIdle idle_msg;
            idle_msg.header_.message_type_ = net_event::Header::new_level_idle;
            pfrm.network_peer().send_message(
                {(byte*)&idle_msg, sizeof idle_msg});

            if (peer_ready_) {
                net_event::NewLevelSyncSeed sync_seed;
                sync_seed.random_state_.set(rng::critical_state);
                sync_seed.difficulty_ = static_cast<u8>(game.difficulty());
                net_event::transmit(pfrm, sync_seed);
                info(pfrm, "sent seed to peer");
            }
        }

        net_event::poll_messages(pfrm, game, *this);

    } else {
        ready_ = true;
    }

    if (ready_) {
        Level next_level = game.level() + 1;

        // Boss levels still need a lot of work before enabling them for
        // multiplayer, in order to properly synchronize the bosses across
        // connected games. For simpler enemies and larger levels, we don't need
        // to be as strict about keeping the enemies perferctly
        // synchronized. But for boss fights, the bar is higher, and I'm not
        // satisfied with any of the progress so far.
        if (is_boss_level(next_level) and pfrm.network_peer().is_connected()) {
            next_level += 1;
        }

        // For now, to determine whether the game's complete, scan through a
        // bunch of levels. If there are no more bosses remaining, the game is
        // complete.
        bool bosses_remaining = false;
        for (Level l = next_level; l < next_level + 1000; ++l) {
            if (is_boss_level(l)) {
                bosses_remaining = true;
                break;
            }
        }

        if (not bosses_remaining) {
            pfrm.sleep(150);
            pfrm.speaker().play_music("waves", 0);
            pfrm.sleep(240);
            return state_pool().create<EndingCutsceneState>();
        }

        return state_pool().create<NewLevelState>(next_level);
    }

    return null_state();
}
