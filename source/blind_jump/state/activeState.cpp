#include "globals.hpp"
#include "state_impl.hpp"


void ActiveState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);
    pfrm.screen().fade(0.f);

    if (restore_keystates) {
        pfrm.keyboard().restore_state(*restore_keystates);
        restore_keystates.reset();
    }

    repaint_health_score(
        pfrm, game, &health_, &score_, &dodge_ready_, UIMetric::Align::left);

    const auto screen_tiles = calc_screen_tiles(pfrm);

    dodge_ready_.emplace(
        pfrm,
        game.player().dodges() ? 383 : 387,
        OverlayCoord{1, u8(screen_tiles.y - (5 + game.powerups().size()))});

    repaint_powerups(pfrm,
                     game,
                     true,
                     &health_,
                     &score_,
                     &dodge_ready_,
                     &powerups_,
                     UIMetric::Align::left);
}


void ActiveState::exit(Platform& pfrm, Game& game, State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);

    health_.reset();
    score_.reset();
    dodge_ready_.reset();

    powerups_.clear();

    pfrm.screen().pixelate(0);
}


StatePtr ActiveState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    const bool dodge_was_ready = game.player().dodges();
    game.player().update(pfrm, game, delta);

    const auto player_int_pos = game.player().get_position().cast<s32>();

    const Vec2<TIdx> tile_coords =
        to_tile_coord({player_int_pos.x, player_int_pos.y + 6});

    std::get<BlindJumpGlobalData>(globals()).visited_.set(
        tile_coords.x, tile_coords.y, true);

    const auto t = pfrm.get_tile(Layer::map_1, tile_coords.x, tile_coords.y);
    switch (t) {
    default:
        break;
    }

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
                      &dodge_ready_,
                      &powerups_,
                      last_health,
                      last_score,
                      dodge_was_ready,
                      UIMetric::Align::left);

    if (game.player().get_health() == 0) {
        pfrm.sleep(5);

        player_death(pfrm, game, game.player().get_position());

        if (pfrm.network_peer().is_connected()) {
            net_event::PlayerDied pd;
            net_event::transmit(pfrm, pd);

            auto next_state =
                state_pool().create<MultiplayerReviveWaitingState>(
                    game.player().get_position());

            game.details().spawn<Signpost>(game.player().get_position(),
                                           Signpost::Type::knocked_out_peer);

            game.player().init({0, 0});
            game.player().set_visible(false);

            // We're dead, hopefully our friend comes to revive us!
            return next_state;
        }

        pfrm.speaker().stop_music();

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
            // In multiplayer mode, we instead repurpose the button mapping to
            // activate a chat window.
            return state_pool().create<QuickChatState>();
        }
    }

    if (pfrm.keyboard().down_transition<quick_select_inventory_key>()) {

        pfrm.speaker().play_sound("openbag", 2);

        return state_pool().create<QuickSelectInventoryState>(game);
    }

    if (pfrm.keyboard().down_transition<quick_map_key>()) {
        if (game.persistent_data().settings_.button_mode_ ==
                Settings::ButtonMode::strafe_combined or
            (not pfrm.keyboard().pressed<Key::left>() and
             not pfrm.keyboard().pressed<Key::right>() and
             not pfrm.keyboard().pressed<Key::up>() and
             not pfrm.keyboard().pressed<Key::down>())) {
            if (game.inventory().item_count(Item::Type::map_system) not_eq 0) {
                // TODO: add a specific sound for opening the map
                pfrm.speaker().play_sound("openbag", 2);

                return state_pool().create<QuickMapState>(game);
            } else {
                push_notification(
                    pfrm,
                    game.state(),
                    locale_string(pfrm, LocaleString::map_required)->c_str());
            }
        }
    }

    if (pfrm.keyboard().down_transition<Key::start>()) {

        if (not pfrm.network_peer().is_connected() and
            not is_boss_level(game.level())) {

            restore_keystates = pfrm.keyboard().dump_state();

            return state_pool().create<PauseScreenState>();
        } else {
            push_notification(
                pfrm,
                game.state(),
                locale_string(pfrm, LocaleString::menu_disabled)->c_str());
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

            game.player().init(t_pos);
            pfrm.speaker().play_sound("bell", 5);
            const auto c = current_zone(game).energy_glow_color_;

            return state_pool().create<PreFadePauseState>(game, c);
        }
    }

    const auto player_pos = game.player().get_position();

    for (auto& sp : game.details().get<Signpost>()) {
        auto pos = sp->get_position();

        const auto dist_heuristic = manhattan_length(player_pos, pos) < 32;

        if (dist_heuristic and distance(player_pos, pos) < 24) {
            if (length(game.effects().get<DialogBubble>()) == 0) {
                auto dialog_pos = pos;
                dialog_pos.y -= 6;
                game.effects().spawn<DialogBubble>(dialog_pos, *sp);
            }
            if (pfrm.keyboard().down_transition(game.action2_key())) {
                game.effects().get<DialogBubble>().clear();
                return state_pool().create<DialogState>(
                    sp->resume_state(pfrm, game), sp->get_dialog());
            }
        } else {
            if (length(game.effects().get<DialogBubble>())) {
                (*game.effects().get<DialogBubble>().begin())->try_destroy(*sp);
            }
        }
    }

    if (game.scavenger() and game.scavenger()->visible()) {
        auto dialog_pos = game.scavenger()->get_position();
        dialog_pos.y -= 9;

        auto sc_pos = game.scavenger()->get_position();
        sc_pos.y += 16;

        const auto action_key = game.persistent_data().settings_.action2_key_;

        const auto dist_heuristic = manhattan_length(player_pos, sc_pos) < 32;

        if (dist_heuristic and distance(player_pos, sc_pos) < 24) {

            if (length(game.effects().get<DialogBubble>()) == 0) {
                game.effects().spawn<DialogBubble>(dialog_pos,
                                                   *game.scavenger());
            }

            if (pfrm.keyboard().down_transition(action_key) and
                not pfrm.keyboard()
                        .any_pressed<Key::left,
                                     Key::right,
                                     Key::up,
                                     Key::down>()) {

                game.effects().get<DialogBubble>().pop();

                auto future_state = make_deferred_state<ItemShopState>();
                static const LocaleString scavenger_dialog[] = {
                    LocaleString::scavenger_shop, LocaleString::empty};
                return state_pool().create<DialogState>(future_state,
                                                        scavenger_dialog);
            }
        } else {
            if (length(game.effects().get<DialogBubble>())) {
                (*game.effects().get<DialogBubble>().begin())
                    ->try_destroy(*game.scavenger());
            }
        }
    }

    return null_state();
}
