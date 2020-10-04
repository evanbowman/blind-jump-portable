#include "state_impl.hpp"


void ActiveState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.screen().fade(0.f);

    if (restore_keystates) {
        pfrm.keyboard().restore_state(*restore_keystates);
        restore_keystates.reset();
    }

    repaint_health_score(pfrm, game, &health_, &score_, UIMetric::Align::left);

    repaint_powerups(
        pfrm, game, true, &health_, &score_, &powerups_, UIMetric::Align::left);
}


void ActiveState::exit(Platform& pfrm, Game& game, State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);

    health_.reset();
    score_.reset();

    powerups_.clear();

    pfrm.screen().pixelate(0);
}


StatePtr ActiveState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().update(pfrm, game, delta);

    const auto player_int_pos = game.player().get_position().cast<s32>();

    const Vec2<TIdx> tile_coords =
        to_tile_coord({player_int_pos.x, player_int_pos.y + 6});
    visited.set(tile_coords.x, tile_coords.y, true);

    const auto& player_spr = game.player().get_sprite();
    if (auto amt = player_spr.get_mix().amount_) {
        if (player_spr.get_mix().color_ ==
            current_zone(game).injury_glow_color_) {
            pfrm.screen().pixelate(amt / 8, false);
            pixelated_ = true;
        }
    } else if (pixelated_) {
        pfrm.screen().pixelate(0);
        pixelated_ = false;
    }

    const auto last_health = game.player().get_health();
    const auto last_score = game.score();

    if (auto new_state = OverworldState::update(pfrm, game, delta)) {
        return new_state;
    }

    update_ui_metrics(pfrm,
                      game,
                      delta,
                      &health_,
                      &score_,
                      &powerups_,
                      last_health,
                      last_score,
                      UIMetric::Align::left);

    if (game.player().get_health() == 0) {
        pfrm.sleep(5);
        pfrm.speaker().stop_music();
        // TODO: add a unique explosion sound effect
        player_death(pfrm, game, game.player().get_position());

        if (pfrm.network_peer().is_connected()) {
            net_event::PlayerDied pd;
            net_event::transmit(pfrm, pd);

            // Eventually, in a future state, we will want to disconnect our own
            // network peer. But we don't want to disconnect right away,
            // otherwise, the PlayerDied event may not be sent out. So wait
            // until the next state, or the state afterwards.
        }

        return state_pool().create<DeathFadeState>(game);
    }

    if (pfrm.keyboard().down_transition<inventory_key>()) {
        // Menu states disabled in multiplayer mode. You can still use the
        // quickselect inventory though.
        if (not pfrm.network_peer().is_connected()) {

            // We don't update entities, e.g. the player entity, while in the
            // inventory state, so the player will not receive the up_transition
            // keystate unless we push the keystates, and restore after exiting
            // the inventory screen.
            restore_keystates = pfrm.keyboard().dump_state();

            pfrm.speaker().play_sound("openbag", 2);

            return state_pool().create<InventoryState>(true);

        } else {
            push_notification(
                              pfrm, game.state(), locale_string(pfrm, LocaleString::menu_disabled).obj_->c_str());
        }
    }

    if (pfrm.keyboard().down_transition<quick_select_inventory_key>()) {

        pfrm.speaker().play_sound("openbag", 2);

        return state_pool().create<QuickSelectInventoryState>(game);
    }

    if (pfrm.keyboard().down_transition<quick_map_key>()) {
        if (game.inventory().item_count(Item::Type::map_system) not_eq 0) {
            // TODO: add a specific sound for opening the map
            pfrm.speaker().play_sound("openbag", 2);

            return state_pool().create<QuickMapState>(game);
        } else {
            push_notification(
                              pfrm, game.state(), locale_string(pfrm, LocaleString::map_required).obj_->c_str());
        }
    }

    if (pfrm.keyboard().down_transition<Key::start>()) {

        if (not pfrm.network_peer().is_connected() and
            not is_boss_level(game.level())) {

            restore_keystates = pfrm.keyboard().dump_state();

            return state_pool().create<PauseScreenState>();
        } else {
            push_notification(
                              pfrm, game.state(), locale_string(pfrm, LocaleString::menu_disabled).obj_->c_str());
        }
    }

    if (game.transporter().visible()) {

        const auto& t_pos =
            game.transporter().get_position() - Vec2<Float>{0, 22};

        const auto dist =
            apply_gravity_well(t_pos, game.player(), 48, 26, 1.3f, 34);

        if (dist < 6) {
            if (pfrm.network_peer().is_connected()) {
                net_event::PlayerEnteredGate e;
                net_event::transmit(pfrm, e);
            }

            game.player().move(t_pos);
            pfrm.speaker().play_sound("bell", 5);
            const auto c = current_zone(game).energy_glow_color_;
            return state_pool().create<PreFadePauseState>(game, c);
        }
    }

    if (game.scavenger() and game.scavenger()->visible()) {
        auto sc_pos = game.scavenger()->get_position();
        sc_pos.y += 16;

        const auto action_key = game.persistent_data().settings_.action2_key_;

        if (pfrm.keyboard().down_transition(action_key) and
            not pfrm.keyboard()
                    .any_pressed<Key::left, Key::right, Key::up, Key::down>()) {

            const auto dist = distance(game.player().get_position(), sc_pos);
            if (dist < 24) {
                return state_pool().create<ItemShopState>();
            }
        }
    }

    return null_state();
}
