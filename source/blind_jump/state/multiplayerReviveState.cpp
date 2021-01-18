#include "state_impl.hpp"


void MultiplayerReviveState::show_menu(Platform& pfrm)
{
    const auto st = calc_screen_tiles(pfrm);

    const auto len = integer_text_length(donate_health_count_);

    heart_icon_.emplace(
        pfrm, 145, OverlayCoord{u8(st.x / 2 - len / 2 - 1), u8(st.y / 2 - 4)});

    text_.emplace(
        pfrm, "x", OverlayCoord{u8(st.x / 2 - len / 2), u8(st.y / 2 - 4)});
    text_->append(to_string<8>(donate_health_count_).c_str());
}


void MultiplayerReviveState::enter(Platform& pfrm,
                                   Game& game,
                                   State& prev_state)
{
    show_menu(pfrm);

    repaint_health_score(
        pfrm, game, &health_, &score_, nullptr, UIMetric::Align::left);

    repaint_powerups(pfrm,
                     game,
                     true,
                     &health_,
                     &score_,
                     nullptr,
                     &powerups_,
                     UIMetric::Align::left);
}


void MultiplayerReviveState::exit(Platform& pfrm, Game& game, State& next_state)
{
    heart_icon_.reset();
    text_.reset();
    health_.reset();
    score_.reset();
    powerups_.clear();
}


StatePtr
MultiplayerReviveState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    const auto last_health = game.player().get_health();
    const auto last_score = game.score();

    OverworldState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition<Key::up>()) {
        if (game.player().get_health() > 1) {
            ++donate_health_count_;
            game.player().set_health(game.player().get_health() - 1);
            show_menu(pfrm);
        }

        pfrm.speaker().play_sound("scroll", 1);

    } else if (pfrm.keyboard().down_transition<Key::down>()) {
        if (donate_health_count_ > 0) {
            --donate_health_count_;
            game.player().set_health(game.player().get_health() + 1);
            show_menu(pfrm);
        }

        pfrm.speaker().play_sound("scroll", 1);

    } else if (pfrm.keyboard().down_transition(game.action2_key())) {
        if (donate_health_count_ == 0) {
            return state_pool().create<ActiveState>();
        }

        pfrm.speaker().play_sound("select", 1);

        net_event::HealthTransfer hp;
        hp.amount_.set(donate_health_count_);

        net_event::transmit(pfrm, hp);

        auto& sps = game.details().get<Signpost>();
        for (auto it = sps.begin(); it not_eq sps.end();) {
            if ((*it)->get_type() == Signpost::Type::knocked_out_peer) {
                it = sps.erase(it);
            } else {
                ++it;
            }
        }

        return state_pool().create<ActiveState>();
    } else if (pfrm.keyboard().down_transition(game.action1_key())) {
        game.player().set_health(game.player().get_health() +
                                 donate_health_count_);
        return state_pool().create<ActiveState>();
    }

    update_ui_metrics(pfrm,
                      game,
                      delta,
                      &health_,
                      &score_,
                      nullptr,
                      &powerups_,
                      last_health,
                      last_score,
                      false,
                      UIMetric::Align::left);

    return null_state();
}
