#include "state_impl.hpp"
#include "bitvector.hpp"
#include "conf.hpp"
#include "number/random.hpp"


// I know that this is a huge file. But this is basically a giant
// object-oriented state machine, and each state is generally less than 100
// lines, which would make creating files for each state tedious. Furthermore,
// all of the state class instances share a memory pool, and the pool definition
// needs to know about all of the constituent class definitions, so at the very
// least, all of the source files, if split up, would share a giant header file,
// and I would need to make the pool extern... which I would really prefer not
// to do. Also, the game targets embedded systems, like the gameboy advance, and
// putting lots of stuff in a single source file is a tried-and-true method for
// decreasing size of the compiled ROM.


void State::enter(Platform&, Game&, State&)
{
}
void State::exit(Platform&, Game&, State&)
{
}
StatePtr State::update(Platform&, Game&, Microseconds)
{
    return null_state();
}


bool within_view_frustum(const Platform::Screen& screen,
                         const Vec2<Float>& pos);


static Bitmatrix<TileMap::width, TileMap::height> visited;


void show_boss_health(Platform& pfrm, Game& game, Float percentage)
{
    if (auto state = dynamic_cast<ActiveState*>(game.state())) {
        if (not state->boss_health_bar_) {
            state->boss_health_bar_.emplace(
                pfrm, 6, OverlayCoord{u8(pfrm.screen().size().x / 8 - 2), 1});
        }

        state->boss_health_bar_->set_health(percentage);
    }
}


void hide_boss_health(Game& game)
{
    if (auto state = dynamic_cast<ActiveState*>(game.state())) {
        state->boss_health_bar_.reset();
    }
}


void push_notification(Platform& pfrm,
                       State* state,
                       const NotificationStr& string)
{
    pfrm.sleep(3);

    if (auto os = dynamic_cast<OverworldState*>(state)) {
        os->notification_status = OverworldState::NotificationStatus::flash;
        os->notification_str = string;
    }
}


// FIXME: this shouldn't be global...
static std::optional<Platform::Keyboard::RestoreState> restore_keystates;


int PauseScreenState::cursor_loc_ = 0;


static void state_deleter(State* s);

StatePtr null_state()
{
    return {nullptr, state_deleter};
}


template <typename... States> class StatePool {
public:
    template <typename TState, typename... Args> StatePtr create(Args&&... args)
    {
        static_assert(std::disjunction<std::is_same<TState, States>...>(),
                      "State missing from state pool");

        if (auto mem = pool_.get()) {
            new (mem) TState(std::forward<Args>(args)...);

            return {reinterpret_cast<TState*>(mem), state_deleter};
        } else {

            return null_state();
        }
    }

    Pool<std::max({sizeof(States)...}),
         // We should only need memory for two states at any given time: the
         // current state, and the next state.
         3,
         std::max({alignof(States)...})>
        pool_;
};
static StatePool<ActiveState,
                 FadeInState,
                 WarpInState,
                 PreFadePauseState,
                 GlowFadeState,
                 FadeOutState,
                 GoodbyeState,
                 DeathFadeState,
                 InventoryState,
                 NotebookState,
                 ImageViewState,
                 NewLevelState,
                 CommandCodeState,
                 PauseScreenState,
                 MapSystemState,
                 NewLevelIdleState,
                 EditSettingsState,
                 IntroCreditsState,
                 IntroLegalMessage,
                 DeathContinueState,
                 RespawnWaitState,
                 LogfileViewerState,
                 EndingCreditsState,
                 NetworkConnectSetupState,
                 NetworkConnectWaitState,
                 LaunchCutsceneState,
                 QuickSelectInventoryState,
                 SignalJammerSelectorState>
    state_pool_;


static void state_deleter(State* s)
{
    if (s) {
        s->~State();
        state_pool_.pool_.post(reinterpret_cast<byte*>(s));
    }
}


////////////////////////////////////////////////////////////////////////////////
// OverworldState
////////////////////////////////////////////////////////////////////////////////


void OverworldState::exit(Platform& pfrm, Game&, State&)
{
    notification_text.reset();
    fps_text_.reset();
    network_tx_msg_text_.reset();
    network_rx_msg_text_.reset();
    network_tx_loss_text_.reset();
    network_rx_loss_text_.reset();

    // In case we're in the middle of an entry/exit animation for the
    // notification bar.
    for (int i = 0; i < 32; ++i) {
        pfrm.set_tile(Layer::overlay, i, 0, 0);
    }

    notification_status = NotificationStatus::hidden;
}


void OverworldState::receive(const net_event::PlayerSpawnLaser& p,
                             Platform&,
                             Game& game)
{
    Vec2<Float> position;
    position.x = p.x_.get();
    position.y = p.y_.get();

    game.effects().spawn<PeerLaser>(position, p.dir_, Laser::Mode::normal);
}


void OverworldState::receive(const net_event::SyncSeed& s, Platform&, Game&)
{
    rng::critical_state = s.random_state_.get();
}


void OverworldState::receive(const net_event::EnemyStateSync& s,
                             Platform&,
                             Game& game)
{
    if (is_boss_level(game.level())) {
        for (auto& e : game.enemies().get<TheFirstExplorer>()) {
            if (e->id() == s.id_.get()) {
                e->sync(s, game);
                return;
            }
        }
    }

    for (auto& e : game.enemies().get<Turret>()) {
        if (e->id() == s.id_.get()) {
            e->sync(s, game);
            return;
        }
    }
    for (auto& e : game.enemies().get<Dasher>()) {
        if (e->id() == s.id_.get()) {
            e->sync(s);
            return;
        }
    }
    for (auto& e : game.enemies().get<Scarecrow>()) {
        if (e->id() == s.id_.get()) {
            e->sync(s);
        }
    }
    for (auto& e : game.enemies().get<Drone>()) {
        if (e->id() == s.id_.get()) {
            e->sync(s);
            return;
        }
    }
}


void OverworldState::receive(const net_event::PlayerInfo& p,
                             Platform&,
                             Game& game)
{
    if (game.peer()) {
        game.peer()->sync(game, p);
    }
}


void OverworldState::receive(const net_event::EnemyHealthChanged& hc,
                             Platform& pfrm,
                             Game& game)
{
    game.enemies().transform([&](auto& buf) {
        for (auto& e : buf) {
            if (e->id() == hc.id_.get()) {
                e->health_changed(hc, pfrm, game);
            }
        }
    });
}


static void transmit_player_info(Platform& pfrm, Game& game)
{
    net_event::PlayerInfo info;
    info.header_.message_type_ = net_event::Header::player_info;
    info.x_.set(game.player().get_position().cast<s16>().x);
    info.y_.set(game.player().get_position().cast<s16>().y);
    info.set_texture_index(game.player().get_sprite().get_texture_index());
    info.set_sprite_size(game.player().get_sprite().get_size());
    info.x_speed_ = game.player().get_speed().x * 10;
    info.y_speed_ = game.player().get_speed().y * 10;
    info.set_visible(game.player().get_sprite().get_alpha() not_eq
                     Sprite::Alpha::transparent);
    info.set_weapon_drawn(game.player().weapon().get_sprite().get_alpha() not_eq
                          Sprite::Alpha::transparent);

    auto mix = game.player().get_sprite().get_mix();
    if (mix.amount_) {
        auto& zone_info = current_zone(game);
        if (mix.color_ == zone_info.injury_glow_color_) {
            info.set_display_color(
                net_event::PlayerInfo::DisplayColor::injured);
        } else if (mix.color_ == zone_info.energy_glow_color_) {
            info.set_display_color(
                net_event::PlayerInfo::DisplayColor::got_coin);
        } else if (mix.color_ == ColorConstant::spanish_crimson) {
            info.set_display_color(
                net_event::PlayerInfo::DisplayColor::got_heart);
        }
        info.set_color_amount(mix.amount_);
    } else {
        info.set_display_color(net_event::PlayerInfo::DisplayColor::none);
        info.set_color_amount(0);
    }

    pfrm.network_peer().send_message({(byte*)&info, sizeof info});
}


void OverworldState::multiplayer_sync(Platform& pfrm,
                                      Game& game,
                                      Microseconds delta)
{
    // On the gameboy advance, we're dealing with a slow connection and
    // twenty-year-old technology, so, realistically, we can only transmit
    // player data a few times per second. TODO: add methods for querying the
    // uplink performance limits from the Platform class, rather than having
    // various game-specific hacks here.

    // FIXME: I strongly suspect that this number can be increased without
    // issues, but I'm waiting until I test on an actual device with a link
    // cable.
    static const auto player_refresh_rate = seconds(1) / 20;

    static auto update_counter = player_refresh_rate;

    if (not game.peer()) {
        game.peer().emplace();
    }

    update_counter -= delta;
    if (update_counter <= 0) {
        update_counter = player_refresh_rate;

        transmit_player_info(pfrm, game);

        if (pfrm.network_peer().is_host()) {
            net_event::SyncSeed s;
            s.random_state_.set(rng::critical_state);
            net_event::transmit(pfrm, s);
        }
    }


    net_event::poll_messages(pfrm, game, *this);


    game.peer()->update(pfrm, game, delta);
}


static void
player_death(Platform& pfrm, Game& game, const Vec2<Float>& position)
{
    pfrm.speaker().play_sound("explosion1", 3, position);
    big_explosion(pfrm, game, position);
}


void OverworldState::show_stats(Platform& pfrm, Game& game, Microseconds delta)
{
    fps_timer_ += delta;
    fps_frame_count_ += 1;

    if (fps_timer_ >= seconds(1)) {
        fps_timer_ -= seconds(1);

        fps_text_.emplace(pfrm, OverlayCoord{1, 2});
        network_tx_msg_text_.emplace(pfrm, OverlayCoord{1, 3});
        network_rx_msg_text_.emplace(pfrm, OverlayCoord{1, 4});
        network_tx_loss_text_.emplace(pfrm, OverlayCoord{1, 5});
        network_rx_loss_text_.emplace(pfrm, OverlayCoord{1, 6});

        const auto colors =
            fps_frame_count_ < 55
                ? Text::OptColors{{ColorConstant::rich_black,
                                   ColorConstant::aerospace_orange}}
                : Text::OptColors{};

        fps_text_->assign(fps_frame_count_, colors);
        fps_text_->append(" fps", colors);
        fps_frame_count_ = 0;

        const auto net_stats = pfrm.network_peer().stats();

        const auto tx_loss_colors =
            net_stats.transmit_loss_ > 0
                ? Text::OptColors{{ColorConstant::rich_black,
                                   ColorConstant::aerospace_orange}}
                : Text::OptColors{};

        const auto rx_loss_colors =
            net_stats.receive_loss_ > 0
                ? Text::OptColors{{ColorConstant::rich_black,
                                   ColorConstant::aerospace_orange}}
                : Text::OptColors{};

        network_tx_msg_text_->append(net_stats.transmit_count_);
        network_tx_msg_text_->append(
            locale_string(LocaleString::network_tx_stats_suffix));
        network_rx_msg_text_->append(net_stats.receive_count_);
        network_rx_msg_text_->append(
            locale_string(LocaleString::network_rx_stats_suffix));
        network_tx_loss_text_->append(net_stats.transmit_loss_, tx_loss_colors);
        network_tx_loss_text_->append(
            locale_string(LocaleString::network_tx_loss_stats_suffix),
            tx_loss_colors);
        network_rx_loss_text_->append(net_stats.receive_loss_, rx_loss_colors);
        network_rx_loss_text_->append(
            locale_string(LocaleString::network_rx_loss_stats_suffix),
            rx_loss_colors);
    }
}


StatePtr OverworldState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    animate_starfield(pfrm, delta);

    if (pfrm.network_peer().is_connected()) {
        multiplayer_sync(pfrm, game, delta);
    } else if (game.peer()) {
        player_death(pfrm, game, game.peer()->get_position());
        game.peer().reset();
        push_notification(
            pfrm, game.state(), locale_string(LocaleString::peer_lost));
    } else {
        // In multiplayer mode, we need to synchronize the random number
        // engine. In single-player mode, let's advance the rng for each step,
        // to add unpredictability
        rng::get(rng::critical_state);
    }

    if (game.persistent_data().settings_.show_stats_) {
        show_stats(pfrm, game, delta);
    } else if (fps_text_) {
        fps_text_.reset();
        network_tx_msg_text_.reset();
        network_tx_loss_text_.reset();
        network_rx_loss_text_.reset();

        fps_frame_count_ = 0;
        fps_timer_ = 0;
    }

    Player& player = game.player();

    auto update_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                (*it)->on_death(pfrm, game);
                it = entity_buf.erase(it);
            } else {
                (*it)->update(pfrm, game, delta);
                ++it;
            }
        }
    };

    game.effects().transform(update_policy);
    game.details().transform(update_policy);

    auto enemy_timestep = delta;
    if (auto lethargy = get_powerup(game, Powerup::Type::lethargy)) {
        if (lethargy->parameter_ > 0) {
            // FIXME: Improve this code! Division! Yikes!
            const auto last = lethargy->parameter_ / 1000000;
            lethargy->parameter_ -= delta;
            const auto current = lethargy->parameter_ / 1000000;
            if (current not_eq last) {
                lethargy->dirty_ = true;
            }
            enemy_timestep /= 2;
        }
    }

    if (pfrm.keyboard().up_transition<Key::action_1>()) {
        camera_snap_timer_ = seconds(2) + milliseconds(250);
    }

    if (camera_snap_timer_ > 0) {
        if (pfrm.keyboard().down_transition<Key::action_2>()) {
            camera_snap_timer_ = 0;
        }
        camera_snap_timer_ -= delta;
    }

    bool enemies_remaining = false;
    bool enemies_destroyed = false;
    bool enemies_visible = false;
    game.enemies().transform([&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                (*it)->on_death(pfrm, game);
                it = entity_buf.erase(it);
                enemies_destroyed = true;
            } else {
                enemies_remaining = true;

                (*it)->update(pfrm, game, enemy_timestep);

                if (camera_tracking_ and
                    (pfrm.keyboard().pressed<Key::action_1>() or
                     camera_snap_timer_ > 0)) {
                    // NOTE: snake body segments do not make much sense to
                    // center the camera on, so exclude them. Same for various
                    // other enemies...
                    using T = typename std::remove_reference<decltype(
                        entity_buf)>::type;

                    using VT = typename T::ValueType::element_type;

                    if constexpr (not std::is_same<VT, SnakeBody>() and
                                  not std::is_same<VT, SnakeHead>() and
                                  not std::is_same<VT, GatekeeperShield>()) {
                        if ((*it)->visible()) {
                            enemies_visible = true;
                            game.camera().push_ballast((*it)->get_position());
                        }
                    }
                }
                ++it;
            }
        }
    });

    if (not enemies_visible) {
        camera_snap_timer_ = 0;
    }

    if (not enemies_remaining and enemies_destroyed) {

        NotificationStr str;
        str += locale_string(LocaleString::level_clear);

        push_notification(pfrm, game.state(), str);
    }


    switch (notification_status) {
    case NotificationStatus::hidden:
        break;

    case NotificationStatus::flash:
        for (int x = 0; x < 32; ++x) {
            pfrm.set_tile(Layer::overlay, x, 0, 108);
        }
        notification_status = NotificationStatus::flash_animate;
        notification_text_timer = -1 * milliseconds(5);
        break;

    case NotificationStatus::flash_animate:
        notification_text_timer += delta;
        if (notification_text_timer > milliseconds(10)) {
            notification_text_timer = 0;

            const auto current_tile = pfrm.get_tile(Layer::overlay, 0, 0);
            if (current_tile < 110) {
                for (int x = 0; x < 32; ++x) {
                    pfrm.set_tile(Layer::overlay, x, 0, current_tile + 1);
                }
            } else {
                notification_status = NotificationStatus::wait;
                notification_text_timer = milliseconds(80);
            }
        }
        break;

    case NotificationStatus::wait:
        notification_text_timer -= delta;
        if (notification_text_timer <= 0) {
            notification_text_timer = seconds(3);

            notification_text.emplace(pfrm, OverlayCoord{0, 0});

            const auto margin =
                centered_text_margins(pfrm, notification_str.length());

            left_text_margin(*notification_text, margin);
            notification_text->append(notification_str.c_str());
            right_text_margin(*notification_text, margin);

            notification_status = NotificationStatus::display;
        }
        break;

    case NotificationStatus::display:
        if (notification_text_timer > 0) {
            notification_text_timer -= delta;

        } else {
            notification_text_timer = 0;
            notification_status = NotificationStatus::exit;

            for (int x = 0; x < 32; ++x) {
                pfrm.set_tile(Layer::overlay, x, 0, 112);
            }
        }
        break;

    case NotificationStatus::exit: {
        notification_text_timer += delta;
        if (notification_text_timer > milliseconds(34)) {
            notification_text_timer = 0;

            const auto tile = pfrm.get_tile(Layer::overlay, 0, 0);
            if (tile < 120) {
                for (int x = 0; x < 32; ++x) {
                    pfrm.set_tile(Layer::overlay, x, 0, tile + 1);
                }
            } else {
                notification_status = NotificationStatus::hidden;
                notification_text.reset();
            }
        }
        break;
    }
    }

    game.camera().update(pfrm, delta, player.get_position());


    check_collisions(pfrm, game, player, game.details().get<Item>());
    check_collisions(pfrm, game, player, game.effects().get<OrbShot>());

    if (UNLIKELY(is_boss_level(game.level()))) {
        check_collisions(
            pfrm, game, player, game.effects().get<FirstExplorerBigLaser>());
        check_collisions(
            pfrm, game, player, game.effects().get<FirstExplorerSmallLaser>());
    }

    game.enemies().transform([&](auto& buf) {
        using T = typename std::remove_reference<decltype(buf)>::type;
        using VT = typename T::ValueType::element_type;

        if (pfrm.network_peer().is_connected()) {
            check_collisions(pfrm, game, game.effects().get<PeerLaser>(), buf);
        }

        check_collisions(pfrm, game, game.effects().get<AlliedOrbShot>(), buf);

        if constexpr (not std::is_same<Scarecrow, VT>() and
                      not std::is_same<SnakeTail, VT>() and
                      not std::is_same<Sinkhole, VT>()) {
            check_collisions(pfrm, game, player, buf);
        }

        if constexpr (not std::is_same<Sinkhole, VT>()) {
            check_collisions(pfrm, game, game.effects().get<Laser>(), buf);
        }
    });

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// ActiveState
////////////////////////////////////////////////////////////////////////////////


static void repaint_health_score(Platform& pfrm,
                                 Game& game,
                                 std::optional<UIMetric>* health,
                                 std::optional<UIMetric>* score)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    health->emplace(
        pfrm,
        OverlayCoord{1, u8(screen_tiles.y - (3 + game.powerups().size()))},
        145,
        (int)game.player().get_health());

    score->emplace(
        pfrm,
        OverlayCoord{1, u8(screen_tiles.y - (2 + game.powerups().size()))},
        146,
        game.score());
}


static void repaint_powerups(Platform& pfrm,
                             Game& game,
                             bool clean,
                             std::optional<UIMetric>* health,
                             std::optional<UIMetric>* score,
                             Buffer<UIMetric, Powerup::max_>* powerups)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    if (clean) {

        repaint_health_score(pfrm, game, health, score);

        powerups->clear();

        u8 write_pos = screen_tiles.y - 2;

        for (auto& powerup : game.powerups()) {
            // const u8 off = integer_text_length(powerup.parameter_) + 1;
            powerups->emplace_back(pfrm,
                                   OverlayCoord{1, write_pos},
                                   powerup.icon_index(),
                                   powerup.parameter_);

            powerup.dirty_ = false;

            --write_pos;
        }
    } else {
        for (u32 i = 0; i < game.powerups().size(); ++i) {
            if (game.powerups()[i].dirty_) {
                switch (game.powerups()[i].display_mode_) {
                case Powerup::DisplayMode::integer:
                    (*powerups)[i].set_value(game.powerups()[i].parameter_);
                    break;

                case Powerup::DisplayMode::timestamp:
                    (*powerups)[i].set_value(game.powerups()[i].parameter_ /
                                             1000000);
                    break;
                }
                game.powerups()[i].dirty_ = false;
            }
        }
    }
}


static void update_powerups(Platform& pfrm,
                            Game& game,
                            std::optional<UIMetric>* health,
                            std::optional<UIMetric>* score,
                            Buffer<UIMetric, Powerup::max_>* powerups)
{
    bool update_powerups = false;
    bool update_all = false;
    for (auto it = game.powerups().begin(); it not_eq game.powerups().end();) {
        if (it->parameter_ <= 0) {
            it = game.powerups().erase(it);
            update_powerups = true;
            update_all = true;
        } else {
            if (it->dirty_) {
                update_powerups = true;
            }
            ++it;
        }
    }

    if (update_powerups) {
        repaint_powerups(pfrm, game, update_all, health, score, powerups);
    }
}


static void update_ui_metrics(Platform& pfrm,
                              Game& game,
                              Microseconds delta,
                              std::optional<UIMetric>* health,
                              std::optional<UIMetric>* score,
                              Buffer<UIMetric, Powerup::max_>* powerups,
                              Entity::Health last_health,
                              Score last_score)
{
    update_powerups(pfrm, game, health, score, powerups);

    if (last_health not_eq game.player().get_health()) {
        (*health)->set_value(game.player().get_health());
    }

    if (last_score not_eq game.score()) {
        (*score)->set_value(game.score());
    }

    (*health)->update(pfrm, delta);
    (*score)->update(pfrm, delta);

    for (auto& powerup : *powerups) {
        powerup.update(pfrm, delta);
    }
}


void ActiveState::enter(Platform& pfrm, Game& game, State&)
{
    if (restore_keystates) {
        pfrm.keyboard().restore_state(*restore_keystates);
        restore_keystates.reset();
    }

    repaint_health_score(pfrm, game, &health_, &score_);

    repaint_powerups(pfrm, game, true, &health_, &score_, &powerups_);
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

    OverworldState::update(pfrm, game, delta);

    update_ui_metrics(pfrm,
                      game,
                      delta,
                      &health_,
                      &score_,
                      &powerups_,
                      last_health,
                      last_score);

    if (game.player().get_health() == 0) {
        pfrm.sleep(5);
        pfrm.speaker().stop_music();
        // TODO: add a unique explosion sound effect
        player_death(pfrm, game, game.player().get_position());

        if (pfrm.network_peer().is_connected()) {
            pfrm.network_peer().disconnect();
        }

        return state_pool_.create<DeathFadeState>(game);
    }

    if (pfrm.keyboard().down_transition<Key::alt_1>()) {

        // Menu states disabled in multiplayer mode. You can still use the
        // quickselect inventory though.
        if (not pfrm.network_peer().is_connected()) {

            // We don't update entities, e.g. the player entity, while in the
            // inventory state, so the player will not receive the up_transition
            // keystate unless we push the keystates, and restore after exiting
            // the inventory screen.
            restore_keystates = pfrm.keyboard().dump_state();

            pfrm.speaker().play_sound("openbag", 2);

            return state_pool_.create<InventoryState>(true);

        } else {
            push_notification(
                pfrm, game.state(), locale_string(LocaleString::menu_disabled));
        }
    }

    if (pfrm.keyboard().down_transition<Key::alt_2>()) {

        restore_keystates = pfrm.keyboard().dump_state();

        pfrm.speaker().play_sound("openbag", 2);

        return state_pool_.create<QuickSelectInventoryState>(game);
    }

    if (pfrm.keyboard().down_transition<Key::start>()) {

        if (not pfrm.network_peer().is_connected() and
            not is_boss_level(game.level())) {
            restore_keystates = pfrm.keyboard().dump_state();

            return state_pool_.create<PauseScreenState>();
        } else {
            push_notification(
                pfrm, game.state(), locale_string(LocaleString::menu_disabled));
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
            return state_pool_.create<PreFadePauseState>(game);
        }
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// FadeInState
////////////////////////////////////////////////////////////////////////////////


void FadeInState::enter(Platform& pfrm, Game& game, State&)
{
    game.player().set_visible(game.level() == 0);
    game.camera().set_speed(0.75);

    if (game.peer()) {
        game.peer()->warping() = true;
    }
}


void FadeInState::exit(Platform& pfrm, Game& game, State&)
{
    game.camera().set_speed(1.f);
}


StatePtr FadeInState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(800);
    if (counter_ > fade_duration) {
        if (game.level() == 0) {
            return state_pool_.create<ActiveState>(game);
        } else {
            pfrm.screen().fade(1.f, current_zone(game).energy_glow_color_);
            pfrm.sleep(2);
            game.player().set_visible(true);

            return state_pool_.create<WarpInState>(game);
        }
    } else {
        const auto amount = 1.f - smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// WarpInState
////////////////////////////////////////////////////////////////////////////////


StatePtr WarpInState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    if (counter_ > milliseconds(200) and not shook_) {
        game.camera().shake();
        shook_ = true;
    }

    constexpr auto fade_duration = milliseconds(950);
    if (counter_ > fade_duration) {
        shook_ = false;
        pfrm.screen().fade(0.f, current_zone(game).energy_glow_color_);
        pfrm.screen().pixelate(0);

        if (game.peer()) {
            game.peer()->warping() = false;
        }

        return state_pool_.create<ActiveState>(game);
    } else {
        const auto amount = 1.f - smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, current_zone(game).energy_glow_color_);
        if (amount > 0.5f) {
            pfrm.screen().pixelate((amount - 0.5f) * 60);
        }
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// PreFadePauseState
////////////////////////////////////////////////////////////////////////////////


StatePtr
PreFadePauseState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.camera().set_speed(0.75f);

    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    if (manhattan_length(pfrm.screen().get_view().get_center() +
                             pfrm.screen().get_view().get_size() / 2.f,
                         game.player().get_position()) < 18) {
        game.camera().set_speed(1.f);
        return state_pool_.create<GlowFadeState>(game);
    } else {
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// GlowFadeState
////////////////////////////////////////////////////////////////////////////////


StatePtr GlowFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(950);
    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f, current_zone(game).energy_glow_color_);
        pfrm.screen().pixelate(0);
        game.player().set_visible(false);
        return state_pool_.create<FadeOutState>(game);
    } else {
        const auto amount = smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, current_zone(game).energy_glow_color_);
        if (amount > 0.25f) {
            pfrm.screen().pixelate((amount - 0.25f) * 60);
        }
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// FadeOutState
////////////////////////////////////////////////////////////////////////////////


StatePtr FadeOutState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(670);
    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f);

        return state_pool_.create<NewLevelIdleState>();

    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           current_zone(game).energy_glow_color_);

        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// DeathFadeState
////////////////////////////////////////////////////////////////////////////////


void DeathFadeState::enter(Platform& pfrm, Game& game, State& prev_state)
{
}


StatePtr DeathFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = seconds(2);
    if (counter_ > fade_duration) {
        pfrm.screen().pixelate(0);

        auto screen_tiles = calc_screen_tiles(pfrm);

        const auto image_width = 18;

        draw_image(pfrm,
                   450,
                   (screen_tiles.x - image_width) / 2,
                   3,
                   image_width,
                   3,
                   Layer::overlay);

        return state_pool_.create<DeathContinueState>();
    } else {
        const auto amount = smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, current_zone(game).injury_glow_color_);
        pfrm.screen().pixelate(amount * 100);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// RespawnState
////////////////////////////////////////////////////////////////////////////////


void RespawnWaitState::enter(Platform& pfrm, Game& game, State&)
{
    // pfrm.speaker().play_sound("bell", 5);
}


StatePtr
RespawnWaitState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    if (counter_ > milliseconds(800)) {
        return state_pool_.create<LaunchCutsceneState>();
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// DeathContinueState
////////////////////////////////////////////////////////////////////////////////


void DeathContinueState::enter(Platform& pfrm, Game& game, State&)
{
    game.player().set_visible(false);
}


void DeathContinueState::exit(Platform& pfrm, Game& game, State&)
{
    score_.reset();
    highscore_.reset();
    level_.reset();
    items_collected_.reset();

    pfrm.fill_overlay(0);
}


StatePtr
DeathContinueState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    constexpr auto fade_duration = seconds(1);
    if (counter_ > fade_duration) {
        counter_ -= delta;
        pfrm.screen().fade(1.f,
                           ColorConstant::rich_black,
                           current_zone(game).injury_glow_color_);

        constexpr auto stats_time = milliseconds(500);

        if (counter2_ < stats_time) {
            const bool show_stats = counter2_ + delta > stats_time;

            counter2_ += delta;

            if (show_stats) {
                score_.emplace(pfrm, Vec2<u8>{1, 8});
                highscore_.emplace(pfrm, Vec2<u8>{1, 10});
                level_.emplace(pfrm, Vec2<u8>{1, 12});
                items_collected_.emplace(pfrm, Vec2<u8>{1, 14});

                const auto screen_tiles = calc_screen_tiles(pfrm);

                auto print_metric = [&](Text& target,
                                        const char* str,
                                        int num,
                                        const char* suffix = "") {
                    target.append(str);

                    const auto iters =
                        screen_tiles.x -
                        (utf8::len(str) + 2 + integer_text_length(num) +
                         utf8::len(suffix));
                    for (u32 i = 0; i < iters; ++i) {
                        target.append(
                            locale_string(LocaleString::punctuation_period));
                    }

                    target.append(num);
                    target.append(suffix);
                };

                game.powerups().clear();

                for (auto& score : reversed(game.highscores())) {
                    if (score < game.score()) {
                        score = game.score();
                        break;
                    }
                }
                std::sort(game.highscores().rbegin(), game.highscores().rend());

                rng::get(rng::critical_state);

                print_metric(
                    *score_, locale_string(LocaleString::score), game.score());
                print_metric(*highscore_,
                             locale_string(LocaleString::high_score),
                             game.highscores()[0]);
                print_metric(*level_,
                             locale_string(LocaleString::waypoints),
                             game.level());
                print_metric(
                    *items_collected_,
                    locale_string(LocaleString::items_collected_prefix),
                    100 * items_collected_percentage(game.inventory()),
                    locale_string(LocaleString::items_collected_suffix));

                game.persistent_data().seed_ = rng::critical_state;
                game.inventory().remove_non_persistent();

                PersistentData& data = game.persistent_data().reset(pfrm);
                pfrm.write_save_data(&data, sizeof data);
            }
        }

        if (score_) {
            if (pfrm.keyboard().pressed<Key::action_1>() or
                pfrm.keyboard().pressed<Key::action_2>()) {

                game.score() = 0;
                game.player().revive(pfrm);

                return state_pool_.create<RespawnWaitState>();
            }
        }
        return null_state();

    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           current_zone(game).injury_glow_color_);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// InventoryState
////////////////////////////////////////////////////////////////////////////////


// These are static, because it's simply more convenient for the player when the inventory page and selector are persistent across opening/closing the inventory page.
int InventoryState::page_{0};
Vec2<u8> InventoryState::selector_coord_{0, 0};


struct InventoryItemHandler {
    Item::Type type_;
    int icon_;
    StatePtr (*callback_)(Platform& pfrm, Game& game);
    LocaleString description_;
    enum { no = 0, yes, custom } single_use_ = no;
};


static void consume_selected_item(Game& game)
{
    game.inventory().remove_item(InventoryState::page_,
                                 InventoryState::selector_coord_.x,
                                 InventoryState::selector_coord_.y);
}


constexpr int item_icon(Item::Type item)
{
    int icon_base = 177;

    return icon_base +
           (static_cast<int>(item) - (static_cast<int>(Item::Type::coin) + 1)) *
               4;
}


// Just so we don't have to type so much stuff. Some items will invariably use
// icons from existing items, so we still want the flexibility of setting icon
// values manually.
#define STANDARD_ITEM_HANDLER(TYPE)                                            \
    Item::Type::TYPE, item_icon(Item::Type::TYPE)


constexpr static const InventoryItemHandler inventory_handlers[] = {
    {Item::Type::null,
     0,
     [](Platform&, Game&) { return null_state(); },
     LocaleString::empty_inventory_str},
    {STANDARD_ITEM_HANDLER(old_poster_1),
     [](Platform&, Game&) {
         static const auto str = "old_poster_flattened";
         return state_pool_.create<ImageViewState>(str,
                                                   ColorConstant::steel_blue);
     },
     LocaleString::old_poster_title},
    {STANDARD_ITEM_HANDLER(surveyor_logbook),
     [](Platform&, Game&) {
         return state_pool_.create<NotebookState>(
             locale_string(LocaleString::logbook_str_1));
     },
     LocaleString::surveyor_logbook_title},
    {STANDARD_ITEM_HANDLER(blaster),
     [](Platform&, Game&) { return null_state(); },
     LocaleString::blaster_title},
    {STANDARD_ITEM_HANDLER(accelerator),
     [](Platform&, Game& game) {
         add_powerup(game, Powerup::Type::accelerator, 60);
         return null_state();
     },
     LocaleString::accelerator_title,
     InventoryItemHandler::yes},
    {STANDARD_ITEM_HANDLER(lethargy),
     [](Platform&, Game& game) {
         add_powerup(game,
                     Powerup::Type::lethargy,
                     seconds(18),
                     Powerup::DisplayMode::timestamp);
         return null_state();
     },
     LocaleString::lethargy_title,
     InventoryItemHandler::yes},
    {STANDARD_ITEM_HANDLER(map_system),
     [](Platform&, Game&) { return state_pool_.create<MapSystemState>(); },
     LocaleString::map_system_title},
    {STANDARD_ITEM_HANDLER(explosive_rounds_2),
     [](Platform&, Game& game) {
         add_powerup(game, Powerup::Type::explosive_rounds, 2);
         return null_state();
     },
     LocaleString::explosive_rounds_title,
     InventoryItemHandler::yes},
    {STANDARD_ITEM_HANDLER(seed_packet),
     [](Platform&, Game&) {
         static const auto str = "seed_packet_flattened";
         return state_pool_.create<ImageViewState>(str,
                                                   ColorConstant::steel_blue);
     },
     LocaleString::seed_packet_title},
    {STANDARD_ITEM_HANDLER(engineer_notebook),
     [](Platform&, Game&) {
         return state_pool_.create<NotebookState>(
             locale_string(LocaleString::engineer_notebook_str));
     },
     LocaleString::engineer_notebook_title},
    {STANDARD_ITEM_HANDLER(signal_jammer),
     [](Platform&, Game&) {
         return state_pool_.create<SignalJammerSelectorState>();
         return null_state();
     },
     LocaleString::signal_jammer_title,
     InventoryItemHandler::custom},
    {STANDARD_ITEM_HANDLER(navigation_pamphlet),
     [](Platform&, Game&) {
         return state_pool_.create<NotebookState>(
             locale_string(LocaleString::navigation_pamphlet));
     },
     LocaleString::navigation_pamphlet_title}};


static const InventoryItemHandler* inventory_item_handler(Item::Type type)
{
    for (auto& handler : inventory_handlers) {
        if (handler.type_ == type) {
            return &handler;
        }
    }
    return nullptr;
}


const char* item_description(Item::Type type)
{
    if (auto handler = inventory_item_handler(type)) {
        return locale_string(handler->description_);

    } else {
        return nullptr;
    }
}


StatePtr InventoryState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition<Key::alt_1>()) {
        return state_pool_.create<ActiveState>(game);
    }

    selector_timer_ += delta;

    constexpr auto fade_duration = milliseconds(400);
    if (fade_timer_ < fade_duration) {
        fade_timer_ += delta;

        if (fade_timer_ >= fade_duration) {
            page_text_.emplace(pfrm, OverlayCoord{1, 1});
            page_text_->assign(page_ + 1);
            update_item_description(pfrm, game);
            display_items(pfrm, game);
        }

        pfrm.screen().fade(smoothstep(0.f, fade_duration, fade_timer_));
    }

    if (fade_timer_ > fade_duration / 2) {

        if (pfrm.keyboard().down_transition<Key::left>()) {
            if (selector_coord_.x > 0) {
                selector_coord_.x -= 1;
            } else {
                if (page_ > 0) {
                    page_ -= 1;
                    selector_coord_.x = 4;
                    if (page_text_) {
                        page_text_->assign(page_ + 1);
                    }
                    update_arrow_icons(pfrm);
                    display_items(pfrm, game);
                }
            }
            update_item_description(pfrm, game);

        } else if (pfrm.keyboard().down_transition<Key::right>()) {
            if (selector_coord_.x < 4) {
                selector_coord_.x += 1;
            } else {
                if (page_ < Inventory::pages - 1) {
                    page_ += 1;
                    selector_coord_.x = 0;
                    if (page_text_) {
                        page_text_->assign(page_ + 1);
                    }
                    update_arrow_icons(pfrm);
                    display_items(pfrm, game);
                }
            }
            update_item_description(pfrm, game);

        } else if (pfrm.keyboard().down_transition<Key::down>()) {
            if (selector_coord_.y < 1) {
                selector_coord_.y += 1;
            }
            update_item_description(pfrm, game);

        } else if (pfrm.keyboard().down_transition<Key::up>()) {
            if (selector_coord_.y > 0) {
                selector_coord_.y -= 1;
            }
            update_item_description(pfrm, game);
        }
    }

    if (pfrm.keyboard().down_transition<Key::action_2>()) {

        const auto item = game.inventory().get_item(
            page_, selector_coord_.x, selector_coord_.y);

        if (auto handler = inventory_item_handler(item)) {

            if (handler->single_use_ == InventoryItemHandler::yes) {
                consume_selected_item(game);
            }

            if (auto new_state = handler->callback_(pfrm, game)) {
                return new_state;
            } else {
                update_item_description(pfrm, game);
                display_items(pfrm, game);
            }
        }
    }

    if (selector_timer_ > milliseconds(75)) {
        selector_timer_ = 0;
        const OverlayCoord pos{static_cast<u8>(3 + selector_coord_.x * 5),
                               static_cast<u8>(3 + selector_coord_.y * 5)};
        if (selector_shaded_) {
            selector_.emplace(pfrm, OverlayCoord{4, 4}, pos, false, 8);
            selector_shaded_ = false;
        } else {
            selector_.emplace(pfrm, OverlayCoord{4, 4}, pos, false, 16);
            selector_shaded_ = true;
        }
    }

    return null_state();
}


InventoryState::InventoryState(bool fade_in)
{
    if (not fade_in) {
        fade_timer_ = fade_duration_ - Microseconds{1};
    }
}


static void draw_dot_grid(Platform& pfrm)
{
    for (int i = 0; i < 6; ++i) {
        pfrm.set_tile(Layer::overlay, 2 + i * 5, 2, 176);
        pfrm.set_tile(Layer::overlay, 2 + i * 5, 7, 176);
        pfrm.set_tile(Layer::overlay, 2 + i * 5, 12, 176);
    }
}


void InventoryState::enter(Platform& pfrm, Game& game, State&)
{
    update_arrow_icons(pfrm);
    draw_dot_grid(pfrm);
}


void InventoryState::exit(Platform& pfrm, Game& game, State& next_state)
{
    // Un-fading and re-fading when switching from the inventory view to an item
    // view creates tearing. Only unfade the screen when switching back to the
    // active state.
    if (dynamic_cast<ActiveState*>(&next_state)) {
        pfrm.screen().fade(0.f);
    }

    selector_.reset();
    left_icon_.reset();
    right_icon_.reset();
    page_text_.reset();
    item_description_.reset();
    item_description2_.reset();
    label_.reset();

    pfrm.fill_overlay(0);

    clear_items();
}


void InventoryState::update_arrow_icons(Platform& pfrm)
{
    switch (page_) {
    case 0:
        right_icon_.emplace(pfrm, 172, OverlayCoord{28, 7});
        left_icon_.emplace(pfrm, 175, OverlayCoord{1, 7});
        break;

    case Inventory::pages - 1:
        right_icon_.emplace(pfrm, 174, OverlayCoord{28, 7});
        left_icon_.emplace(pfrm, 173, OverlayCoord{1, 7});
        break;

    default:
        right_icon_.emplace(pfrm, 172, OverlayCoord{28, 7});
        left_icon_.emplace(pfrm, 173, OverlayCoord{1, 7});
        break;
    }
}


void InventoryState::update_item_description(Platform& pfrm, Game& game)

{
    const auto item =
        game.inventory().get_item(page_, selector_coord_.x, selector_coord_.y);

    constexpr static const OverlayCoord text_loc{3, 15};

    if (auto handler = inventory_item_handler(item)) {
        item_description_.emplace(
            pfrm, locale_string(handler->description_), text_loc);
        item_description_->append(".");

        if (handler->single_use_) {
            item_description2_.emplace(
                pfrm, OverlayCoord{text_loc.x, text_loc.y + 2});

            item_description2_->assign(
                locale_string(LocaleString::single_use_warning),
                FontColors{ColorConstant::med_blue_gray,
                           ColorConstant::rich_black});
        } else {
            item_description2_.reset();
        }
    }
}


void InventoryState::clear_items()
{
    for (auto& items : item_icons_) {
        for (auto& item : items) {
            item.reset();
        }
    }
}


void InventoryState::display_items(Platform& pfrm, Game& game)
{
    clear_items();

    auto screen_tiles = calc_screen_tiles(pfrm);

    const char* label_str = locale_string(LocaleString::items);
    label_.emplace(
        pfrm, OverlayCoord{u8(screen_tiles.x - (utf8::len(label_str) + 1)), 1});
    label_->assign(label_str);

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 2; ++j) {

            const auto item = game.inventory().get_item(page_, i, j);

            const OverlayCoord coord{static_cast<u8>(4 + i * 5),
                                     static_cast<u8>(4 + j * 5)};

            if (item not_eq Item::Type::null) {
                if (auto handler = inventory_item_handler(item)) {
                    item_icons_[i][j].emplace(pfrm, handler->icon_, coord);
                }
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// QuickSelectInventoryState
////////////////////////////////////////////////////////////////////////////////


void QuickSelectInventoryState::enter(Platform& pfrm,
                                      Game& game,
                                      State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);

    sidebar_.emplace(pfrm, 6);
    sidebar_->set_display_percentage(0.f);

    repaint_health_score(pfrm, game, &health_, &score_);
}


void QuickSelectInventoryState::exit(Platform& pfrm,
                                     Game& game,
                                     State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);

    health_.reset();
    score_.reset();
    powerups_.clear();
    item_icons_.clear();

    sidebar_.reset();
    selector_.reset();
    sidebar_->set_display_percentage(0.f);
}


void QuickSelectInventoryState::draw_items(Platform& pfrm, Game& game)
{
    items_.clear();
    item_icons_.clear();

    auto screen_tiles = calc_screen_tiles(pfrm);

    for (int page = 0; page < Inventory::pages; ++page) {
        for (int col = 0; col < Inventory::cols; ++col) {
            for (int row = 0; row < Inventory::rows; ++row) {
                if (item_icons_.full()) {
                    return;
                }
                const auto item = game.inventory().get_item(page, col, row);
                if (auto handler = inventory_item_handler(item)) {
                    if (handler->single_use_) {
                        items_.push_back(item);

                        const OverlayCoord coord{
                            static_cast<u8>(screen_tiles.x - 4),
                            static_cast<u8>(4 + item_icons_.size() * 5)};

                        item_icons_.emplace_back(pfrm, handler->icon_, coord);
                    }
                }
            }
        }
    }
}


void QuickSelectInventoryState::show_sidebar(Platform& pfrm)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    sidebar_->set_display_percentage(1.f);

    for (int y = 0; y < screen_tiles.y; y += 5) {
        pfrm.set_tile(Layer::overlay, screen_tiles.x - 1, y + 2, 129);
        pfrm.set_tile(Layer::overlay, screen_tiles.x - 6, y + 2, 129);
    }

    pfrm.set_tile(Layer::overlay, screen_tiles.x - 3, 1, 151);
    pfrm.set_tile(Layer::overlay, screen_tiles.x - 3, screen_tiles.y - 2, 152);
}


StatePtr QuickSelectInventoryState::update(Platform& pfrm,
                                           Game& game,
                                           Microseconds delta)
{
    const auto last_health = game.player().get_health();
    const auto last_score = game.score();

    OverworldState::update(pfrm, game, delta);

    update_ui_metrics(pfrm,
                      game,
                      delta,
                      &health_,
                      &score_,
                      &powerups_,
                      last_health,
                      last_score);


    game.player().soft_update(pfrm, game, delta);

    static const auto transition_duration = milliseconds(160);

    switch (display_mode_) {
    case DisplayMode::enter: {
        timer_ += delta;
        if (timer_ >= transition_duration) {
            timer_ = 0;
            display_mode_ = DisplayMode::show;

            show_sidebar(pfrm);

            draw_items(pfrm, game);

        } else {
            if (not pfrm.keyboard().pressed<Key::alt_2>()) {
                timer_ = transition_duration - timer_;
                display_mode_ = DisplayMode::exit;
            } else {
                const auto amount =
                    smoothstep(0.f, transition_duration, timer_);

                sidebar_->set_display_percentage(amount);
                pfrm.screen().fade(0.25f * amount);
            }
        }
        break;
    }

    case DisplayMode::show: {

        timer_ += delta;

        auto redraw_selector = [&](TileDesc erase_color) {
            auto screen_tiles = calc_screen_tiles(pfrm);

            const OverlayCoord pos{static_cast<u8>(screen_tiles.x - 5),
                                   static_cast<u8>(3 + selector_pos_ * 5)};
            if (selector_shaded_) {
                selector_.emplace(
                    pfrm, OverlayCoord{4, 4}, pos, false, 8 + 278, erase_color);
                selector_shaded_ = false;
            } else {
                selector_.emplace(pfrm,
                                  OverlayCoord{4, 4},
                                  pos,
                                  false,
                                  16 + 278,
                                  erase_color);
                selector_shaded_ = true;
            }
        };

        if (pfrm.keyboard().down_transition<Key::up>()) {
            if (selector_pos_ > 0) {
                selector_pos_ -= 1;
            }
            redraw_selector(112);
        } else if (pfrm.keyboard().down_transition<Key::down>()) {
            if (selector_pos_ < items_.capacity() - 1) {
                selector_pos_ += 1;
            }
            redraw_selector(112);
        } else if (pfrm.keyboard().down_transition<Key::action_2>()) {
            if (selector_pos_ < items_.size()) {
                for (int page = 0; page < Inventory::pages; ++page) {
                    for (int col = 0; col < Inventory::cols; ++col) {
                        for (int row = 0; row < Inventory::rows; ++row) {
                            if (game.inventory().get_item(page, col, row) ==
                                items_[selector_pos_]) {
                                game.inventory().remove_item(page, col, row);
                                goto DONE;
                            }
                        }
                    }
                }
            DONE:
                if (auto handler =
                        inventory_item_handler(items_[selector_pos_])) {
                    handler->callback_(pfrm, game);
                }
                draw_items(pfrm, game);
                show_sidebar(pfrm); // Because MediumIcon resets tiles back to
                                    // zero, thus punching a hole in the
                                    // sidebar.
                draw_items(pfrm, game);


                repaint_powerups(
                    pfrm, game, true, &health_, &score_, &powerups_);
                redraw_selector(112);
            }
        }

        if (timer_ > milliseconds(75)) {
            timer_ = 0;
            redraw_selector(112);
        }

        if (not pfrm.keyboard().pressed<Key::alt_2>()) {
            display_mode_ = DisplayMode::exit;
            timer_ = 0;
            redraw_selector(0);
        }
        break;
    }

    case DisplayMode::exit: {
        timer_ += delta;
        if (timer_ >= transition_duration) {
            timer_ = 0;
            return state_pool_.create<ActiveState>(game);
        } else {
            if (pfrm.keyboard().pressed<Key::alt_2>()) {
                timer_ = transition_duration - timer_;
                display_mode_ = DisplayMode::enter;
            } else {
                const auto amount =
                    1.f - smoothstep(0.f, transition_duration, timer_);

                sidebar_->set_display_percentage(amount);
                pfrm.screen().fade(0.25f * amount);
            }
        }
        break;
    }
    }


    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// NotebookState
////////////////////////////////////////////////////////////////////////////////

constexpr u16 notebook_margin_tile = 82;

NotebookState::NotebookState(const char* text) : str_(text), page_(0)
{
}


void NotebookState::enter(Platform& pfrm, Game&, State&)
{
    // pfrm.speaker().play_sound("open_book", 0);

    pfrm.sleep(1); // Well, this is embarassing... basically, the fade function
                   // creates tearing on the gameboy, and we can mitigate the
                   // tearing with a sleep call, which is implemented to wait on
                   // a vblank interrupt. I could defer the fade on those
                   // platforms to run after the vblank, but fade is cpu
                   // intensive, and sometimes ends up exceeding the blank
                   // period, causing tearing anyway.

    pfrm.screen().fade(1.f, ColorConstant::rich_black, {}, true, true);
    pfrm.load_overlay_texture("overlay_journal");
    pfrm.screen().fade(1.f, ColorConstant::aged_paper, {}, true, true);

    auto screen_tiles = calc_screen_tiles(pfrm);
    text_.emplace(pfrm);
    text_->assign(str_,
                  {1, 2},
                  OverlayCoord{u8(screen_tiles.x - 2), u8(screen_tiles.y - 4)});
    page_number_.emplace(pfrm, OverlayCoord{0, u8(screen_tiles.y - 1)});

    // repaint_page(pfrm);
}


void NotebookState::repaint_margin(Platform& pfrm)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    for (int x = 0; x < screen_tiles.x; ++x) {
        for (int y = 0; y < screen_tiles.y; ++y) {
            if (x == 0 or y == 0 or y == 1 or x == screen_tiles.x - 1 or
                y == screen_tiles.y - 2 or y == screen_tiles.y - 1) {
                pfrm.set_tile(Layer::overlay, x, y, notebook_margin_tile);
            }
        }
    }
}


void NotebookState::repaint_page(Platform& pfrm)
{
    const auto size = text_->size();

    page_number_->erase();
    repaint_margin(pfrm);
    page_number_->assign(page_ + 1);
    text_->assign(str_, {1, 2}, size, page_ * (size.y / 2));
}


void NotebookState::exit(Platform& pfrm, Game&, State&)
{
    pfrm.sleep(1);
    pfrm.fill_overlay(0); // The TextView destructor cleans up anyway, but we
                          // have ways of clearing the screen faster than the
                          // TextView implementation is able to. The TextView
                          // class needs to loop through each glyph and
                          // individually zero them out, which can create
                          // tearing in the display. The fill_overlay() function
                          // doesn't need to work around sub-regions of the
                          // screen, so it can use faster methods, like a single
                          // memset, or special BIOS calls (depending on the
                          // platform) to clear out the screen.

    pfrm.screen().fade(1.f);

    text_.reset();
    pfrm.load_overlay_texture("overlay");
}


StatePtr NotebookState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition<Key::action_2>()) {
        return state_pool_.create<InventoryState>(false);
    }

    switch (display_mode_) {
    case DisplayMode::after_transition:
        // NOTE: Loading a notebook page for the first time results in a large
        // amount of lag on some systems, which effectively skips the screen
        // fade. The first load of a new page causes the platform to load a
        // bunch of glyphs into memory for the first time, which means copying
        // over texture memory, possibly adjusting a glyph mapping table,
        // etc. So we're just going to sit for one update cycle, and let the
        // huge delta pass over.
        display_mode_ = DisplayMode::fade_in;
        break;

    case DisplayMode::fade_in:
        static const auto fade_duration = milliseconds(200);
        timer_ += delta;
        if (timer_ < fade_duration) {

            pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, timer_),
                               ColorConstant::aged_paper,
                               {},
                               true,
                               true);
        } else {
            timer_ = 0;
            pfrm.screen().fade(0.f);
            display_mode_ = DisplayMode::show;
        }
        break;

    case DisplayMode::show:
        if (pfrm.keyboard().down_transition<Key::down>()) {
            if (text_->parsed() not_eq utf8::len(str_)) {
                page_ += 1;
                timer_ = 0;
                display_mode_ = DisplayMode::fade_out;
            }

        } else if (pfrm.keyboard().down_transition<Key::up>()) {
            if (page_ > 0) {
                page_ -= 1;
                timer_ = 0;
                display_mode_ = DisplayMode::fade_out;
            }
        }
        break;

    case DisplayMode::fade_out: {
        timer_ += delta;
        static const auto fade_duration = milliseconds(200);
        if (timer_ < fade_duration) {
            pfrm.screen().fade(smoothstep(0.f, fade_duration, timer_),
                               ColorConstant::aged_paper,
                               {},
                               true,
                               true);
        } else {
            timer_ = 0;
            pfrm.screen().fade(1.f, ColorConstant::aged_paper, {}, true, true);
            display_mode_ = DisplayMode::transition;
        }
        break;
    }

    case DisplayMode::transition:
        repaint_page(pfrm);
        pfrm.speaker().play_sound("open_book", 0);
        display_mode_ = DisplayMode::after_transition;
        break;
    }

    return null_state();
}

////////////////////////////////////////////////////////////////////////////////
// ImageViewState
////////////////////////////////////////////////////////////////////////////////


ImageViewState::ImageViewState(const char* image_name,
                               ColorConstant background_color)
    : image_name_(image_name), background_color_(background_color)
{
}


StatePtr ImageViewState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition<Key::action_2>()) {
        return state_pool_.create<InventoryState>(false);
    }

    return null_state();
}


void ImageViewState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.enable_glyph_mode(false);

    pfrm.sleep(1);
    pfrm.screen().fade(1.f, background_color_);
    pfrm.load_overlay_texture(image_name_);

    const auto screen_tiles = calc_screen_tiles(pfrm);

    draw_image(
        pfrm, 1, 1, 1, screen_tiles.x - 2, screen_tiles.y - 3, Layer::overlay);
}


void ImageViewState::exit(Platform& pfrm, Game& game, State&)
{
    pfrm.sleep(1);
    pfrm.fill_overlay(0);
    pfrm.sleep(1);
    pfrm.screen().fade(1.f);

    pfrm.enable_glyph_mode(true);
    pfrm.load_overlay_texture("overlay");
}


////////////////////////////////////////////////////////////////////////////////
// MapSystemState
////////////////////////////////////////////////////////////////////////////////


void MapSystemState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.screen().fade(1.f);

    const auto dname = pfrm.device_name();
    map_enter_duration_ = milliseconds(
        Conf(pfrm).expect<Conf::Integer>(dname.c_str(), "minimap_enter_time"));
}


void MapSystemState::exit(Platform& pfrm, Game& game, State&)
{
    pfrm.screen().fade(1.f);
    pfrm.fill_overlay(0);
    level_text_.reset();
    for (auto& text : legend_text_) {
        text.reset();
    }
    legend_border_.reset();
}


StatePtr MapSystemState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    auto screen_tiles = calc_screen_tiles(pfrm);

    auto set_tile = [&](s8 x, s8 y, int icon, bool dodge = true) {
        const auto tile = pfrm.get_tile(Layer::overlay, x + 1, y);
        if (dodge and (tile == 133 or tile == 132)) {
            // ...
        } else {
            pfrm.set_tile(Layer::overlay, x + 1, y, icon);
        }
    };

    auto render_map_icon = [&](Entity& entity, s16 icon) {
        auto t = to_tile_coord(entity.get_position().cast<s32>());
        if (is_walkable__fast(game.tiles().get_tile(t.x, t.y))) {
            set_tile(t.x, t.y, icon);
        }
    };

    switch (anim_state_) {
    case AnimState::map_enter: {
        timer_ += delta;
        const auto current_column =
            std::min(TileMap::width,
                     interpolate(TileMap::width,
                                 (decltype(TileMap::width))0,
                                 Float(timer_) / map_enter_duration_));
        game.tiles().for_each([&](Tile t, s8 x, s8 y) {
            if (x > current_column or x <= last_column_) {
                return;
            }
            bool visited_nearby = false;
            static const int offset = 3;
            for (int x2 = std::max(0, x - (offset + 1));
                 x2 < std::min((int)TileMap::width, x + offset + 1);
                 ++x2) {
                for (int y2 = std::max(0, y - offset);
                     y2 < std::min((int)TileMap::height, y + offset);
                     ++y2) {
                    if (visited.get(x2, y2)) {
                        visited_nearby = true;
                    }
                }
            }
            if (not visited_nearby) {
                set_tile(x, y, is_walkable__fast(t) ? 132 : 133, false);
            } else if (is_walkable__fast(t)) {
                set_tile(x, y, 143, false);
            } else {
                if (is_walkable__fast(game.tiles().get_tile(x, y - 1))) {
                    set_tile(x, y, 140);
                } else {
                    set_tile(x, y, 144, false);
                }
            }
        });
        last_column_ = current_column;

        if (current_column == TileMap::width) {
            timer_ = 0;
            anim_state_ = AnimState::map_decorate;
        }
        break;
    }

    case AnimState::map_decorate: {
        timer_ += delta;
        if (timer_ > milliseconds(32)) {
            game.enemies().transform([&](auto& buf) {
                for (auto& entity : buf) {
                    render_map_icon(*entity, 139);
                }
            });

            render_map_icon(game.transporter(), 141);
            for (auto& chest : game.details().get<ItemChest>()) {
                if (chest->state() not_eq ItemChest::State::opened) {
                    render_map_icon(*chest, 138);
                }
            }

            auto player_tile =
                to_tile_coord(game.player().get_position().cast<s32>());
            //u32 integer_text_length(int n);
            if (not is_walkable__fast(
                    game.tiles().get_tile(player_tile.x, player_tile.y))) {
                // Player movement isn't constrained to tiles exactly, and sometimes the
                // player's map icon displays as inside of a wall.
                if (is_walkable__fast(game.tiles().get_tile(player_tile.x + 1,
                                                            player_tile.y))) {
                    player_tile.x += 1;
                } else if (is_walkable__fast(game.tiles().get_tile(
                               player_tile.x, player_tile.y + 1))) {
                    player_tile.y += 1;
                }
            }
            set_tile(player_tile.x, player_tile.y, 142);

            timer_ = 0;
            anim_state_ = AnimState::wp_text;
        }
        break;
    }

    case AnimState::wp_text:
        timer_ += delta;
        if (timer_ > milliseconds(32)) {
            timer_ = 0;
            anim_state_ = AnimState::legend;

            const char* level_str = locale_string(LocaleString::waypoint_text);

            level_text_.emplace(
                pfrm,
                OverlayCoord{
                    u8(screen_tiles.x - (1 + utf8::len(level_str) +
                                         integer_text_length(game.level()))),
                    1});
            level_text_->assign(level_str);
            level_text_->append(game.level());
        }
        break;

    case AnimState::legend:
        timer_ += delta;
        if (timer_ > milliseconds(32)) {
            timer_ = 0;
            anim_state_ = AnimState::wait;

            set_tile(TileMap::width + 2, 11, 137, false); // you
            set_tile(TileMap::width + 2, 13, 135, false); // enemy
            set_tile(TileMap::width + 2, 15, 136, false); // transporter
            set_tile(TileMap::width + 2, 17, 134, false); // item

            legend_border_.emplace(pfrm,
                                   OverlayCoord{11, 9},
                                   OverlayCoord{TileMap::width + 2, 10},
                                   false,
                                   8);

            for (size_t i = 0; i < legend_strings.size(); ++i) {
                const u8 y = 11 + (i * 2);
                legend_text_[i].emplace(pfrm,
                                        locale_string(legend_strings[i]),
                                        OverlayCoord{TileMap::width + 5, y});
            }
        }
        break;

    case AnimState::wait:
        if (pfrm.keyboard().down_transition<Key::action_2>()) {
            return state_pool_.create<InventoryState>(false);
        }
        break;
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// NewLevelState
////////////////////////////////////////////////////////////////////////////////


void NewLevelState::enter(Platform& pfrm, Game& game, State&)
{
    visited.clear();

    pfrm.sleep(15);

    pfrm.screen().fade(1.f);

    const auto s_tiles = calc_screen_tiles(pfrm);

    auto zone = zone_info(next_level_);
    auto last_zone = zone_info(next_level_ - 1);
    if (not(zone == last_zone) or next_level_ == 0) {

        pos_ = OverlayCoord{1, u8(s_tiles.y * 0.3f)};
        text_[0].emplace(pfrm, pos_);

        pos_.y += 2;

        text_[1].emplace(pfrm, pos_);

        const auto l1str = locale_string(zone.title_line_1);
        const auto margin = centered_text_margins(pfrm, utf8::len(l1str));
        left_text_margin(*text_[0], std::max(0, int{margin} - 1));

        text_[0]->append(l1str);

        const auto l2str = locale_string(zone.title_line_2);
        const auto margin2 = centered_text_margins(pfrm, utf8::len(l2str));
        left_text_margin(*text_[1], std::max(0, int{margin2} - 1));

        text_[1]->append(l2str);

        pfrm.sleep(5);

    } else {
        text_[0].emplace(pfrm, OverlayCoord{1, u8(s_tiles.y - 2)});
        text_[0]->append(locale_string(LocaleString::waypoint_text));
        text_[0]->append(next_level_);
        pfrm.sleep(60);
    }
}


static bool startup = true;


StatePtr NewLevelState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    auto zone = zone_info(next_level_);
    auto last_zone = zone_info(next_level_ - 1);

    auto sound_sync_hack = [&pfrm] {
        // FIXME!!!!!! Mysteriously, there's a weird audio glitch, where the
        // sound effects, but not the music, get all glitched out until two
        // sounds are played consecutively. I've spent hours trying to figure
        // out what's going wrong, and I haven't solved this one yet, so for
        // now, just play a couple quiet sounds.
        pfrm.speaker().play_sound("footstep1", 0);
        pfrm.speaker().play_sound("footstep2", 0);
    };


    if (not(zone == last_zone) or next_level_ == 0) {

        timer_ += delta;

        const auto max_j =
            (int)utf8::len(locale_string(zone.title_line_2)) / 2 + 1;
        const auto max_i = max_j * 8;

        const int i = ease_out(timer_, 0, max_i, seconds(1));

        auto repaint = [&pfrm, this](int max_i) {
            while (true) {
                int i = 0, j = 0;
                auto center = calc_screen_tiles(pfrm).x / 2 - 1;

                while (true) {
                    const int y_off = 3;

                    if (max_i > i + 7 + j * 8) {
                        pfrm.set_tile(
                            Layer::overlay, center - j, pos_.y - y_off, 107);
                        pfrm.set_tile(
                            Layer::overlay, center - j, pos_.y + 2, 107);

                        pfrm.set_tile(Layer::overlay,
                                      center + 1 + j,
                                      pos_.y - y_off,
                                      107);
                        pfrm.set_tile(
                            Layer::overlay, center + 1 + j, pos_.y + 2, 107);

                        i = 0;
                        j += 1;
                        continue;
                    }

                    if (j * 8 + i > max_i) {
                        return;
                    }

                    pfrm.set_tile(
                        Layer::overlay, center - j, pos_.y - y_off, 93 + i);
                    pfrm.set_tile(
                        Layer::overlay, center - j, pos_.y + 2, 93 + i);

                    pfrm.set_tile(Layer::overlay,
                                  center + 1 + j,
                                  pos_.y - y_off,
                                  100 + i);
                    pfrm.set_tile(
                        Layer::overlay, center + 1 + j, pos_.y + 2, 100 + i);

                    i++;

                    if (i == 8) {
                        pfrm.set_tile(
                            Layer::overlay, center - j, pos_.y - y_off, 107);
                        pfrm.set_tile(
                            Layer::overlay, center - j, pos_.y + 2, 107);

                        pfrm.set_tile(Layer::overlay,
                                      center + 1 + j,
                                      pos_.y - y_off,
                                      107);
                        pfrm.set_tile(
                            Layer::overlay, center + 1 + j, pos_.y + 2, 107);

                        i = 0;
                        j++;
                    }

                    if (j * 8 + i >= max_i) {
                        return;
                    }
                }
            }
        };

        repaint(std::min(max_i, i));

        if (timer_ > seconds(1)) {
            pfrm.sleep(80);

            pfrm.speaker().play_music(
                zone.music_name_, true, zone.music_offset_);

            if (startup) {
                sound_sync_hack();
            }

            startup = false;
            return state_pool_.create<FadeInState>(game);
        }

    } else {
        // If we're loading from a save state, we need to start the music, which
        // normally starts when we enter a new zone.
        if (startup) {
            pfrm.speaker().play_music(
                zone.music_name_, true, zone.music_offset_);
            sound_sync_hack();
        }
        startup = false;
        return state_pool_.create<FadeInState>(game);
    }

    return null_state();
}


void NewLevelState::exit(Platform& pfrm, Game& game, State&)
{
    game.next_level(pfrm, next_level_);

    // Because generating a level takes quite a bit of time (relative to a
    // normal game update step), and because we aren't really running any game
    // logic during level generation, makes sense to zero-out the delta clock,
    // otherwise the game will go into the next update cycle in the new state
    // with a huge delta time.
    DeltaClock::instance().reset();

    text_[0].reset();
    text_[1].reset();

    pfrm.fill_overlay(0);
}


////////////////////////////////////////////////////////////////////////////////
// IntroCreditsState
////////////////////////////////////////////////////////////////////////////////


void IntroCreditsState::center(Platform& pfrm)
{
    // Because the overlay uses 8x8 tiles, to truely center something, you
    // sometimes need to translate the whole layer.
    if (text_ and text_->len() % 2 == 0) {
        pfrm.set_overlay_origin(-4, 0);
    }
}


void IntroCreditsState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.enable_glyph_mode(true);

    auto pos = (pfrm.screen().size() / u32(8)).cast<u8>();

    // Center horizontally, and place text vertically in top third of screen
    const auto len = utf8::len(str_);
    pos.x -= len + 1;
    pos.x /= 2;
    pos.y *= 0.35f;

    if (len % 2 == 0) {
        pfrm.set_overlay_origin(-4, 0);
    }
    text_.emplace(pfrm, str_, pos);

    center(pfrm);

    pfrm.screen().fade(1.f);
}


void IntroCreditsState::exit(Platform& pfrm, Game& game, State&)
{
    text_.reset();
    pfrm.set_overlay_origin(0, 0);
}


StatePtr IntroLegalMessage::next_state(Platform& pfrm, Game& game)
{
    return state_pool_.create<IntroCreditsState>(
        locale_string(LocaleString::intro_text_2));
}


StatePtr IntroCreditsState::next_state(Platform& pfrm, Game& game)
{
    // backdoor for debugging purposes.
    if (pfrm.keyboard().all_pressed<Key::alt_1, Key::alt_2, Key::start>()) {
        return state_pool_.create<CommandCodeState>();
    }

    if (pfrm.keyboard().pressed<Key::start>()) {
        return state_pool_.create<EndingCreditsState>();
    }

    if (game.level() == 0) {
        return state_pool_.create<LaunchCutsceneState>();
    } else {
        return state_pool_.create<NewLevelState>(game.level());
    }
}


StatePtr
IntroCreditsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    center(pfrm);

    timer_ += delta;

    const auto skip = pfrm.keyboard().down_transition<Key::action_2>();

    if (text_) {
        if (timer_ > seconds(2) + milliseconds(500) or skip) {
            text_.reset();
            timer_ = 0;

            if (skip) {
                return next_state(pfrm, game);
            }
        }

    } else {
        if (timer_ > milliseconds(167) + seconds(1)) {
            return next_state(pfrm, game);
        }
    }

    return null_state();
}


StatePtr State::initial()
{
    return state_pool_.create<IntroLegalMessage>();
}


////////////////////////////////////////////////////////////////////////////////
//
// LaunchCutsceneState
//
////////////////////////////////////////////////////////////////////////////////


void LaunchCutsceneState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    for (int x = 0; x < TileMap::width; ++x) {
        for (int y = 0; y < TileMap::height; ++y) {
            pfrm.set_tile(Layer::map_0, x, y, 3);
            pfrm.set_tile(Layer::map_1, x, y, 0);
        }
    }

    auto clear_entities = [&](auto& buf) { buf.clear(); };

    game.enemies().transform(clear_entities);
    game.details().transform(clear_entities);
    game.effects().transform(clear_entities);


    game.camera() = Camera{};

    pfrm.screen().fade(1.f);

    pfrm.load_sprite_texture("spritesheet_launch_anim");
    pfrm.load_overlay_texture("overlay_cutscene");
    pfrm.load_tile0_texture("launch_flattened");
    pfrm.load_tile1_texture("tilesheet_top");

    // pfrm.screen().fade(0.f, ColorConstant::silver_white);

    const Vec2<Float> arbitrary_offscreen_location{1000, 1000};

    game.transporter().set_position(arbitrary_offscreen_location);
    game.player().set_visible(false);

    const auto screen_tiles = calc_screen_tiles(pfrm);

    for (int i = 0; i < screen_tiles.x; ++i) {
        pfrm.set_tile(Layer::overlay, i, 0, 112);
        pfrm.set_tile(Layer::overlay, i, 1, 112);

        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 1, 112);
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 2, 112);
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 3, 112);
    }

    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 5; ++j) {
            pfrm.set_tile(Layer::background, i, j, 2);
        }
    }


    draw_image(pfrm, 61, 0, 5, 30, 12, Layer::background);

    Proxy::SpriteBuffer buf;

    for (int i = 0; i < 3; ++i) {
        buf.emplace_back();
        buf.back().set_position({136 - 24, 40 + 8});
        buf.back().set_size(Sprite::Size::w16_h32);
        buf.back().set_texture_index(i);
    }

    buf[1].set_origin({-16, 0});
    buf[2].set_origin({-32, 0});

    game.effects().spawn<Proxy>(buf);
}


void LaunchCutsceneState::exit(Platform& pfrm, Game& game, State& next_state)
{
    pfrm.sleep(1);
    pfrm.screen().fade(1.f);
    pfrm.fill_overlay(0);
    altitude_text_.reset();

    pfrm.load_overlay_texture("overlay");

    game.details().transform([](auto& buf) { buf.clear(); });
    game.effects().transform([](auto& buf) { buf.clear(); });
}


StatePtr
LaunchCutsceneState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    timer_ += delta;

    if (static_cast<int>(scene_) < static_cast<int>(Scene::within_clouds)) {
        game.camera().update(pfrm,
                             delta,
                             {(float)pfrm.screen().size().x / 2,
                              (float)pfrm.screen().size().y / 2});
    }


    auto update_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                (*it)->on_death(pfrm, game);
                it = entity_buf.erase(it);
            } else {
                (*it)->update(pfrm, game, delta);
                ++it;
            }
        }
    };

    auto do_camera_shake = [&](int magnitude) {
        camera_shake_timer_ -= delta;

        if (camera_shake_timer_ <= 0) {
            camera_shake_timer_ = milliseconds(80);
            game.camera().shake(magnitude);
        }
    };


    game.details().transform(update_policy);

    if (static_cast<int>(scene_) > static_cast<int>(Scene::fade_transition0)) {
        altitude_ += delta * speed_;

        if (--altitude_update_ == 0) {

            auto screen_tiles = calc_screen_tiles(pfrm);

            altitude_update_ = 30;

            const auto units = locale_string(LocaleString::distance_units_feet);

            auto len = integer_text_length(altitude_) + utf8::len(units);

            if (not altitude_text_ or
                (altitude_text_ and altitude_text_->len() not_eq len)) {
                altitude_text_.emplace(
                    pfrm,
                    OverlayCoord{u8((screen_tiles.x - len) / 2),
                                 u8(screen_tiles.y - 2)});
            }
            altitude_text_->assign(altitude_);
            altitude_text_->append(units);
        }
    }

    auto spawn_cloud = [&] {
        const auto swidth = pfrm.screen().size().x;

        // NOTE: just doing this because random numbers are not naturally well
        // distributed, and we don't want the clouds to bunch up too much.
        if (cloud_lane_ == 0) {
            cloud_lane_ = 1;
            const auto offset =
                rng::choice(swidth / 2, rng::critical_state) + 32;
            game.details().spawn<CutsceneCloud>(
                Vec2<Float>{Float(offset), -80});
        } else {
            const auto offset =
                rng::choice(swidth / 2, rng::critical_state) + swidth / 2 + 32;
            game.details().spawn<CutsceneCloud>(
                Vec2<Float>{Float(offset), -80});
            cloud_lane_ = 0;
        }
    };

    auto animate_ship = [&] {
        anim_timer_ += delta;
        static const auto frame_time = milliseconds(170);
        if (anim_timer_ >= frame_time) {
            anim_timer_ -= frame_time;

            if (anim_index_ < 28) {
                anim_index_ += 1;
                for (auto& p : game.effects().get<Proxy>()) {
                    for (int i = 0; i < 3; ++i) {
                        p->buffer()[i].set_texture_index(anim_index_ * 3 + i);
                    }
                }
            }
        }
    };

    switch (scene_) {
    case Scene::fade_in0: {
        constexpr auto fade_duration = milliseconds(400);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(0.f);
            scene_ = Scene::wait;
        } else {
            const auto amount = 1.f - smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(amount);
        }
        animate_ship();
        break;
    }

    case Scene::wait:
        if (timer_ > seconds(4) + milliseconds(510)) {
            timer_ = 0;
            scene_ = Scene::fade_transition0;
        }
        animate_ship();
        break;

    case Scene::fade_transition0: {
        animate_ship();
        constexpr auto fade_duration = milliseconds(680);
        if (timer_ <= fade_duration) {
            const auto amount = smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(amount);
        }

        if (timer_ > fade_duration) {
            timer_ = 0;
            scene_ = Scene::fade_in;

            rng::critical_state = 2022;

            pfrm.screen().fade(1.f, ColorConstant::rich_black, {}, true, true);

            game.effects().transform([](auto& buf) { buf.clear(); });

            pfrm.load_tile0_texture("tilesheet_intro_cutscene_flattened");
            pfrm.load_sprite_texture("spritesheet_intro_clouds");

            for (int i = 0; i < 32; ++i) {
                for (int j = 0; j < 32; ++j) {
                    pfrm.set_tile(Layer::background, i, j, 4);
                }
            }

            const auto screen_tiles = calc_screen_tiles(pfrm);

            for (int i = 0; i < screen_tiles.x; ++i) {
                pfrm.set_tile(Layer::background, i, screen_tiles.y - 2, 18);
            }

            speed_ = 0.0050000f;

            game.details().spawn<CutsceneCloud>(Vec2<Float>{70, 20});
            game.details().spawn<CutsceneCloud>(Vec2<Float>{150, 60});

            game.on_timeout(pfrm, milliseconds(500), [](Platform&, Game& game) {
                game.details().spawn<CutsceneBird>(Vec2<Float>{210, -15}, 0);
                game.details().spawn<CutsceneBird>(Vec2<Float>{180, -20}, 3);
                game.details().spawn<CutsceneBird>(Vec2<Float>{140, -30}, 6);
            });
        }
        break;
    }

    case Scene::fade_in: {
        constexpr auto fade_duration = milliseconds(600);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(0.f);
        } else {
            const auto amount = 1.f - smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(
                amount, ColorConstant::rich_black, {}, true, true);
        }

        if (timer_ > milliseconds(600)) {
            scene_ = Scene::rising;
        }

        // do_camera_shake(2);

        cloud_spawn_timer_ -= delta;
        if (cloud_spawn_timer_ <= 0) {
            cloud_spawn_timer_ = milliseconds(450);

            spawn_cloud();
        }
        break;
    }

    case Scene::rising: {
        if (timer_ > seconds(5)) {
            timer_ = 0;
            scene_ = Scene::enter_clouds;
            speed_ = 0.02f;
        }

        cloud_spawn_timer_ -= delta;
        if (cloud_spawn_timer_ <= 0) {
            cloud_spawn_timer_ = milliseconds(350);

            spawn_cloud();
        }

        break;
    }

    case Scene::enter_clouds: {

        if (timer_ > seconds(3)) {
            do_camera_shake(7);

            pfrm.screen().pixelate(
                interpolate(0, 30, Float(timer_ - seconds(3)) / seconds(1)),
                false);

        } else {
            do_camera_shake(3);
        }

        constexpr auto fade_duration = seconds(4);
        if (timer_ > fade_duration) {
            speed_ = 0.1f;

            pfrm.screen().fade(1.f, ColorConstant::silver_white);
            pfrm.screen().pixelate(0);

            timer_ = 0;
            scene_ = Scene::within_clouds;

            for (int i = 0; i < 32; ++i) {
                for (int j = 0; j < 32; ++j) {
                    pfrm.set_tile(Layer::background, i, j, 1);
                }
            }

            draw_image(pfrm, 151, 0, 8, 30, 9, Layer::background);
            int moon_x = 7;
            int moon_y = 1;
            pfrm.set_tile(Layer::background, moon_x, moon_y, 2);
            pfrm.set_tile(Layer::background, moon_x + 1, moon_y, 3);
            pfrm.set_tile(Layer::background, moon_x, moon_y + 1, 32);
            pfrm.set_tile(Layer::background, moon_x + 1, moon_y + 1, 33);

            game.details().transform([](auto& buf) { buf.clear(); });

        } else {
            const auto amount = smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(amount, ColorConstant::silver_white);

            cloud_spawn_timer_ -= delta;
            if (cloud_spawn_timer_ <= 0) {
                cloud_spawn_timer_ = milliseconds(70);

                spawn_cloud();
            }
        }
        break;
    }

    case Scene::within_clouds:
        if (timer_ > seconds(2) + milliseconds(300)) {
            timer_ = 0;
            scene_ = Scene::exit_clouds;
        }
        break;

    case Scene::exit_clouds: {
        camera_offset_ = interpolate(-50.f, camera_offset_, delta * 0.0000005f);

        game.camera().set_position(pfrm, {0, camera_offset_});

        if (timer_ > seconds(3)) {
            scene_ = Scene::scroll;
        } else {
            speed_ =
                interpolate(0.00900f, 0.095000f, timer_ / Float(seconds(3)));
        }

        constexpr auto fade_duration = milliseconds(1000);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(0.f);
        } else {
            const auto amount = 1.f - smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(amount, ColorConstant::silver_white);
        }
        break;
    }

    case Scene::scroll: {

        camera_offset_ -= delta * 0.0000014f;

        game.camera().set_position(pfrm, {0, camera_offset_});

        if (timer_ > seconds(8)) {
            timer_ = 0;
            scene_ = Scene::fade_out;
        }

        break;
    }

    case Scene::fade_out: {

        constexpr auto fade_duration = milliseconds(2000);
        if (timer_ > fade_duration) {
            altitude_text_.reset();

            pfrm.screen().fade(1.f);

            pfrm.sleep(5);

            return state_pool_.create<NewLevelState>(Level{0});

        } else {
            camera_offset_ -= delta * 0.0000014f;
            game.camera().set_position(pfrm, {0, camera_offset_});

            const auto amount = smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(
                amount, ColorConstant::rich_black, {}, true, true);
        }
        break;
    }
    }

    if (pfrm.keyboard().down_transition<Key::action_2>()) {
        return state_pool_.create<NewLevelState>(Level{0});
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
//
// EditSettingsState
//
////////////////////////////////////////////////////////////////////////////////


void EditSettingsState::message(Platform& pfrm, const char* str)
{
    str_ = str;

    auto screen_tiles = calc_screen_tiles(pfrm);

    message_anim_.emplace(pfrm, OverlayCoord{0, u8(screen_tiles.y - 1)});

    message_anim_->init(screen_tiles.x);

    pfrm.set_overlay_origin(0, 0);
}


EditSettingsState::EditSettingsState()
    : lines_{{{dynamic_camera_line_updater_},
              {difficulty_line_updater_},
              {show_stats_line_updater_},
              {contrast_line_updater_},
              {language_line_updater_}}}
{
}


void EditSettingsState::draw_line(Platform& pfrm, int row, const char* value)
{
    const int value_len = utf8::len(value);
    const int field_len = utf8::len(locale_string(strings[row]));

    const auto margin = centered_text_margins(pfrm, value_len + field_len + 2);

    lines_[row].text_.emplace(pfrm, OverlayCoord{0, u8(4 + row * 2)});

    left_text_margin(*lines_[row].text_, margin);
    lines_[row].text_->append(locale_string(strings[row]));
    lines_[row].text_->append("  ");
    lines_[row].text_->append(value);

    lines_[row].cursor_begin_ = margin + field_len;
    lines_[row].cursor_end_ = margin + field_len + 2 + value_len + 1;
}


void EditSettingsState::refresh(Platform& pfrm, Game& game)
{
    for (u32 i = 0; i < lines_.size(); ++i) {
        draw_line(pfrm, i, lines_[i].updater_.update(pfrm, game, 0).c_str());
    }
}


void EditSettingsState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    refresh(pfrm, game);
}


void EditSettingsState::exit(Platform& pfrm, Game& game, State& next_state)
{
    for (auto& l : lines_) {
        l.text_.reset();
    }

    message_.reset();

    pfrm.fill_overlay(0);

    pfrm.set_overlay_origin(0, 0);
}


StatePtr
EditSettingsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition<Key::action_2>()) {
        return state_pool_.create<PauseScreenState>(false);
    }

    if (message_anim_ and not message_anim_->done()) {
        message_anim_->update(delta);

        if (message_anim_->done()) {
            message_anim_.reset();

            auto screen_tiles = calc_screen_tiles(pfrm);
            const auto len = utf8::len(str_);
            message_.emplace(pfrm,
                             str_,
                             OverlayCoord{u8((screen_tiles.x - len) / 2),
                                          u8(screen_tiles.y - 1)});
        }
    }

    auto erase_selector = [&] {
        for (u32 i = 0; i < lines_.size(); ++i) {
            const auto& line = lines_[i];
            pfrm.set_tile(
                Layer::overlay, line.cursor_begin_, line.text_->coord().y, 0);
            pfrm.set_tile(
                Layer::overlay, line.cursor_end_, line.text_->coord().y, 0);
        }
    };

    if (pfrm.keyboard().down_transition<Key::down>()) {
        message_.reset();
        if (select_row_ < static_cast<int>(lines_.size() - 1)) {
            select_row_ += 1;
        }
    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        message_.reset();
        if (select_row_ > 0) {
            select_row_ -= 1;
        }
    } else if (pfrm.keyboard().down_transition<Key::right>()) {
        message_.reset();

        erase_selector();

        auto& updater = lines_[select_row_].updater_;

        draw_line(pfrm, select_row_, updater.update(pfrm, game, 1).c_str());

        updater.complete(pfrm, game, *this);

    } else if (pfrm.keyboard().down_transition<Key::left>()) {
        message_.reset();

        erase_selector();

        auto& updater = lines_[select_row_].updater_;

        draw_line(pfrm, select_row_, updater.update(pfrm, game, -1).c_str());

        updater.complete(pfrm, game, *this);
    }

    if (not message_ and not message_anim_) {
        const auto& line = lines_[select_row_];
        const Float y_center = pfrm.screen().size().y / 2;
        const Float y_line = line.text_->coord().y * 8;
        const auto y_diff = (y_line - y_center) * 0.4f;

        y_offset_ = interpolate(Float(y_diff), y_offset_, delta * 0.00001f);

        pfrm.set_overlay_origin(0, y_offset_);
    }

    anim_timer_ += delta;
    if (anim_timer_ > milliseconds(75)) {
        anim_timer_ = 0;
        if (++anim_index_ > 1) {
            anim_index_ = 0;
        }

        auto [left, right] = [&]() -> Vec2<int> {
            switch (anim_index_) {
            default:
            case 0:
                return {147, 148};
            case 1:
                return {149, 150};
            }
        }();

        erase_selector();

        const auto& line = lines_[select_row_];

        pfrm.set_tile(
            Layer::overlay, line.cursor_begin_, line.text_->coord().y, left);
        pfrm.set_tile(
            Layer::overlay, line.cursor_end_, line.text_->coord().y, right);
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
//
// LogfileViewerState
//
////////////////////////////////////////////////////////////////////////////////


void LogfileViewerState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    repaint(pfrm, 0);
}


void LogfileViewerState::exit(Platform& pfrm, Game& game, State& next_state)
{
    pfrm.screen().fade(0.f);
    pfrm.fill_overlay(0);
}


StatePtr
LogfileViewerState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    auto screen_tiles = calc_screen_tiles(pfrm);

    if (pfrm.keyboard().down_transition<Key::select>()) {
        return state_pool_.create<ActiveState>(game);
    } else if (pfrm.keyboard().down_transition<Key::down>()) {
        offset_ += screen_tiles.x;
        repaint(pfrm, offset_);
    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        if (offset_ > 0) {
            offset_ -= screen_tiles.x;
            repaint(pfrm, offset_);
        }
    } else if (pfrm.keyboard().down_transition<Key::right>()) {
        offset_ += screen_tiles.x * screen_tiles.y;
        repaint(pfrm, offset_);
    } else if (pfrm.keyboard().down_transition<Key::left>()) {
        if (offset_ >= screen_tiles.x * screen_tiles.y) {
            offset_ -= screen_tiles.x * screen_tiles.y;
            repaint(pfrm, offset_);
        }
    }

    return null_state();
}


Platform::TextureCpMapper locale_texture_map();


void LogfileViewerState::repaint(Platform& pfrm, int offset)
{
    constexpr int buffer_size = 600;
    u8 buffer[buffer_size];

    pfrm.logger().read(buffer, offset, buffer_size);

    auto screen_tiles = calc_screen_tiles(pfrm);

    int index = 0;
    for (int j = 0; j < screen_tiles.y; ++j) {
        for (int i = 0; i < screen_tiles.x; ++i) {
            // const int index = i + j * screen_tiles.x;
            if (index < buffer_size) {
                const auto t =
                    pfrm.map_glyph(buffer[index], locale_texture_map());
                pfrm.set_tile(Layer::overlay, i, j, t);
                index += 1;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// CommandCodeState FIXME
//
// This code broke after the transition from ascii to unicode. May need some
// more work. But it's not essential to the game in any way.
//
////////////////////////////////////////////////////////////////////////////////


void CommandCodeState::push_char(Platform& pfrm, Game& game, char c)
{
    input_.push_back(c);

    auto screen_tiles = calc_screen_tiles(pfrm);

    input_text_.emplace(pfrm,
                        OverlayCoord{u8(centered_text_margins(pfrm, 10)),
                                     u8(screen_tiles.y / 2 - 2)});

    input_text_->assign(input_.c_str());

    if (input_.full()) {

        // Why am I even bothering with the way that the cheat system's UI
        // will look!? No one will ever even see it... but I'll know how
        // good/bad it looks, and I guess that's a reason...
        while (not selector_shaded_) {
            update_selector(pfrm, milliseconds(20));
        }

        pfrm.sleep(25);

        input_text_->erase();

        if (handle_command_code(pfrm, game)) {
            input_text_->append(" ACCEPTED");

        } else {
            input_text_->append(" REJECTED");
        }

        pfrm.sleep(25);

        input_text_.reset();

        input_.clear();
    }
}


void CommandCodeState::update_selector(Platform& pfrm, Microseconds dt)
{
    selector_timer_ += dt;
    if (selector_timer_ > milliseconds(75)) {
        selector_timer_ = 0;

        auto screen_tiles = calc_screen_tiles(pfrm);

        const auto margin = centered_text_margins(pfrm, 20) - 1;

        const OverlayCoord selector_pos{u8(margin + selector_index_ * 2),
                                        u8(screen_tiles.y - 4)};

        if (selector_shaded_) {
            selector_.emplace(pfrm, OverlayCoord{3, 3}, selector_pos, false, 8);
            selector_shaded_ = false;
        } else {
            selector_.emplace(
                pfrm, OverlayCoord{3, 3}, selector_pos, false, 16);
            selector_shaded_ = true;
        }
    }
}


StatePtr
CommandCodeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    update_selector(pfrm, delta);

    if (pfrm.keyboard().down_transition<Key::left>()) {
        if (selector_index_ > 0) {
            selector_index_ -= 1;
        } else {
            selector_index_ = 9;
        }

    } else if (pfrm.keyboard().down_transition<Key::right>()) {
        if (selector_index_ < 9) {
            selector_index_ += 1;
        } else {
            selector_index_ = 0;
        }

    } else if (pfrm.keyboard().down_transition<Key::down>()) {
        if (not input_.empty()) {
            while (not input_.full()) {
                input_.push_back(*(input_.end() - 1));
            }

            push_char(pfrm, game, *(input_.end() - 1));
        }
    } else if (pfrm.keyboard().down_transition<Key::action_2>()) {
        push_char(pfrm, game, "0123456789"[selector_index_]);

    } else if (pfrm.keyboard().down_transition<Key::action_1>()) {
        if (input_.empty()) {
            return state_pool_.create<NewLevelState>(next_level_);

        } else {
            input_.pop_back();
            input_text_->assign(input_.c_str());
        }
    }

    return null_state();
}


[[noreturn]] static void factory_reset(Platform& pfrm)
{
    PersistentData data;
    data.magic_ = 0xBAD;
    pfrm.write_save_data(&data, sizeof data);
    pfrm.fatal();
}


[[noreturn]] static void debug_boss_level(Platform& pfrm, Level level)
{
    PersistentData data;
    data.level_ = level;
    data.player_health_ = 6;
    pfrm.write_save_data(&data, sizeof data);
    pfrm.fatal();
}


bool CommandCodeState::handle_command_code(Platform& pfrm, Game& game)
{
    // The command interface interprets the leading three digits as a decimal
    // opcode. Each operation will parse the rest of the trailing digits
    // uniquely, if at all.
    const char* input_str = input_.c_str();

    auto to_num = [](char c) { return int{c} - 48; };

    auto opcode = to_num(input_str[0]) * 100 + to_num(input_str[1]) * 10 +
                  to_num(input_str[2]);

    auto single_parameter = [&]() {
        return to_num(input_str[3]) * 1000000 + to_num(input_str[4]) * 100000 +
               to_num(input_str[5]) * 10000 + to_num(input_str[6]) * 1000 +
               to_num(input_str[7]) * 100 + to_num(input_str[8]) * 10 +
               to_num(input_str[9]);
    };

    switch (opcode) {
    case 222: // Jump to next zone
        for (Level level = next_level_; level < 1000; ++level) {
            auto current_zone = zone_info(level);
            auto next_zone = zone_info(level + 1);

            if (not(current_zone == next_zone)) {
                next_level_ = level + 1;
                return true;
            }
        }
        return false;

    case 999: // Jump to the next boss
        for (Level level = next_level_; level < 1000; ++level) {
            if (is_boss_level(level)) {
                next_level_ = level;
                return true;
            }
        }
        return false;

    // For testing boss levels:
    case 1:
        debug_boss_level(pfrm, boss_0_level);
    case 2:
        debug_boss_level(pfrm, boss_1_level);
    case 3:
        debug_boss_level(pfrm, boss_2_level);

    case 100: // Add player health. The main reason that this command doesn't
              // accept a parameter, is that someone could accidentally overflow
              // the health variable and cause bugs.
        game.player().add_health(100);
        return true;

    case 105: // Maybe you want to add some health, but not affect the gameplay
              // too much.
        game.player().add_health(5);
        return true;

    case 200: { // Get item
        const auto item = single_parameter();
        if (item >= static_cast<int>(Item::Type::count) or
            item <= static_cast<int>(Item::Type::coin)) {
            return false;
        }
        game.inventory().push_item(pfrm, game, static_cast<Item::Type>(item));
        return true;
    }

    case 300: // Get all items
        for (int i = static_cast<int>(Item::Type::coin) + 1;
             i < static_cast<int>(Item::Type::count);
             ++i) {

            const auto item = static_cast<Item::Type>(i);

            if (game.inventory().item_count(item) == 0) {
                game.inventory().push_item(pfrm, game, item);
            }
        }
        return true;

    case 404: // Drop all items
        while (game.inventory().get_item(0, 0, 0) not_eq Item::Type::null) {
            game.inventory().remove_item(0, 0, 0);
        }
        return true;


    default:
        return false;

    case 592:
        factory_reset(pfrm);
    }
}


void CommandCodeState::enter(Platform& pfrm, Game& game, State&)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    const auto margin = centered_text_margins(pfrm, 20) - 1;

    numbers_.emplace(pfrm,
                     OverlayCoord{u8(margin + 1), u8(screen_tiles.y - 3)});

    for (int i = 0; i < 10; ++i) {
        numbers_->append(i);
        numbers_->append(" ");
    }

    entry_box_.emplace(pfrm,
                       OverlayCoord{12, 3},
                       OverlayCoord{u8(centered_text_margins(pfrm, 12)),
                                    u8(screen_tiles.y / 2 - 3)},
                       false,
                       16);

    next_level_ = game.level() + 1;
}


void CommandCodeState::exit(Platform& pfrm, Game& game, State&)
{
    input_text_.reset();
    entry_box_.reset();
    selector_.reset();
    numbers_.reset();
    pfrm.fill_overlay(0);
}


/////////////////////////////////////////////////////////////////////////////////
// EndingCreditsState
////////////////////////////////////////////////////////////////////////////////


void EndingCreditsState::enter(Platform& pfrm, Game& game, State&)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    next_y_ = screen_tiles.y + 2;

    game.on_timeout(pfrm, milliseconds(500), [](Platform& pfrm, Game&) {
        pfrm.speaker().play_music("clair_de_lune", false, 0);
    });
}


void EndingCreditsState::exit(Platform& pfrm, Game& game, State&)
{
    lines_.clear();
    pfrm.set_overlay_origin(0, 0);
    pfrm.speaker().stop_music();
}


// NOTE: '%' characters in the credits will be filled with N '.' characters,
// such that the surrounding text is aligned to either edge of the screen.
//
// FIXME: localize the credits? Nah...
static const std::array<const char*, 32> credits_lines = {
    "Artwork and Source Code by",
    "Evan Bowman",
    "",
    "",
    "Music",
    "Hiraeth%Scott Buckley",
    "Omega%Scott Buckley",
    "Computations%Scott Buckley",
    "September%Kai Engel",
    "Clair De Lune%Chad Crouch",
    "",
    "",
    "Playtesting",
    "Benjamin Casler",
    "",
    "",
    "Special Thanks",
    "My Family",
    "Jasper Vijn (Tonc)",
    "The DevkitARM Project"
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "THE END"};


StatePtr
EndingCreditsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    timer_ += delta;

    if (timer_ > milliseconds([&] {
            if (pfrm.keyboard().pressed<Key::action_2>()) {
                return 15;
            } else {
                return 60;
            }
        }())) {

        timer_ = 0;

        pfrm.set_overlay_origin(0, scroll_++);

        constexpr int tile_height = 8;
        const bool scrolled_two_lines = scroll_ % (tile_height * 2) == 0;

        if (scrolled_two_lines) {

            auto screen_tiles = calc_screen_tiles(pfrm);

            if (scroll_ > ((screen_tiles.y + 2) * tile_height) and
                not lines_.empty()) {
                lines_.erase(lines_.begin());
            }

            if (next_ < int{credits_lines.size() - 1}) {
                const u8 y =
                    next_y_ % 32; // The overlay tile layer is 32x32 tiles.
                next_y_ += 2;

                const auto str = credits_lines[next_++];


                bool contains_fill_char = false;
                utf8::scan(
                    [&contains_fill_char,
                     &pfrm](const utf8::Codepoint& cp, const char*, int) {
                        if (cp == '%') {
                            if (contains_fill_char) {
                                error(pfrm, "two fill chars not allowed");
                                pfrm.fatal();
                            }
                            contains_fill_char = true;
                        }
                    },
                    str,
                    str_len(str));

                if (utf8::len(str) > u32(screen_tiles.x - 2)) {
                    error(pfrm, "credits text too large");
                    pfrm.fatal();
                }

                if (contains_fill_char) {
                    const auto fill =
                        screen_tiles.x - ((utf8::len(str) - 1) + 2);
                    lines_.emplace_back(pfrm, OverlayCoord{1, y});
                    utf8::scan(
                        [this,
                         fill](const utf8::Codepoint&, const char* raw, int) {
                            if (str_len(raw) == 1 and raw[0] == '%') {
                                for (size_t i = 0; i < fill; ++i) {
                                    lines_.back().append(".");
                                }
                            } else {
                                lines_.back().append(raw);
                            }
                        },
                        str,
                        str_len(str));
                } else {
                    const auto len = utf8::len(str);
                    const u8 left_margin = (screen_tiles.x - len) / 2;
                    lines_.emplace_back(pfrm, OverlayCoord{left_margin, y});
                    lines_.back().assign(str);
                }


            } else if (lines_.size() == screen_tiles.y / 4) {
                pfrm.sleep(160);
                factory_reset(pfrm);
            }
        }
    }

    return null_state();
}


/////////////////////////////////////////////////////////////////////////////////
// PauseScreenState
////////////////////////////////////////////////////////////////////////////////


void PauseScreenState::draw_cursor(Platform& pfrm)
{
    // draw_dot_grid(pfrm);

    if (not resume_text_ or not save_and_quit_text_ or not settings_text_ or
        not connect_peer_text_) {
        return;
    }

    auto draw_cursor = [&pfrm](Text* target, int tile1, int tile2) {
        const auto pos = target->coord();
        pfrm.set_tile(Layer::overlay, pos.x - 2, pos.y, tile1);
        pfrm.set_tile(Layer::overlay, pos.x + target->len() + 1, pos.y, tile2);
    };

    auto [left, right] = [&]() -> Vec2<int> {
        switch (anim_index_) {
        default:
        case 0:
            return {147, 148};
        case 1:
            return {149, 150};
        }
    }();

    switch (cursor_loc_) {
    default:
    case 0:
        draw_cursor(&(*resume_text_), left, right);
        draw_cursor(&(*connect_peer_text_), 0, 0);
        draw_cursor(&(*settings_text_), 0, 0);
        draw_cursor(&(*save_and_quit_text_), 0, 0);
        break;

    case 1:
        draw_cursor(&(*resume_text_), 0, 0);
        draw_cursor(&(*connect_peer_text_), 0, 0);
        draw_cursor(&(*settings_text_), left, right);
        draw_cursor(&(*save_and_quit_text_), 0, 0);
        break;

    case 2:
        draw_cursor(&(*resume_text_), 0, 0);
        draw_cursor(&(*connect_peer_text_), left, right);
        draw_cursor(&(*settings_text_), 0, 0);
        draw_cursor(&(*save_and_quit_text_), 0, 0);
        break;

    case 3:
        draw_cursor(&(*resume_text_), 0, 0);
        draw_cursor(&(*connect_peer_text_), 0, 0);
        draw_cursor(&(*settings_text_), 0, 0);
        draw_cursor(&(*save_and_quit_text_), left, right);
        break;
    }
}


void PauseScreenState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    if (dynamic_cast<ActiveState*>(&prev_state)) {
        cursor_loc_ = 0;
    }
}


bool PauseScreenState::connect_peer_option_available(Game& game) const
{
    // For now, don't allow syncing when someone's progressed through any of the
    // levels.
    return game.level() == Level{0} and
           Platform::NetworkPeer::supported_by_device();
}


StatePtr
PauseScreenState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (fade_timer_ < fade_duration) {
        fade_timer_ += delta;

        if (fade_timer_ >= fade_duration) {
            const auto screen_tiles = calc_screen_tiles(pfrm);

            const auto resume_text_len =
                utf8::len(locale_string(LocaleString::menu_resume));

            const auto connect_peer_text_len =
                utf8::len(locale_string(LocaleString::menu_connect_peer));

            const auto settings_text_len =
                utf8::len(locale_string(LocaleString::menu_settings));

            const auto snq_text_len =
                utf8::len(locale_string(LocaleString::menu_save_and_quit));

            const u8 resume_x_loc = (screen_tiles.x - resume_text_len) / 2;
            const u8 cp_x_loc = (screen_tiles.x - connect_peer_text_len) / 2;
            const u8 settings_x_loc = (screen_tiles.x - settings_text_len) / 2;
            const u8 snq_x_loc = (screen_tiles.x - snq_text_len) / 2;

            const u8 y = screen_tiles.y / 2;

            resume_text_.emplace(pfrm, OverlayCoord{resume_x_loc, u8(y - 4)});
            connect_peer_text_.emplace(pfrm, OverlayCoord{cp_x_loc, u8(y - 0)});
            settings_text_.emplace(pfrm,
                                   OverlayCoord{settings_x_loc, u8(y - 2)});
            save_and_quit_text_.emplace(pfrm,
                                        OverlayCoord{snq_x_loc, u8(y + 2)});

            resume_text_->assign(locale_string(LocaleString::menu_resume));

            connect_peer_text_->assign(
                locale_string(LocaleString::menu_connect_peer),
                [&]() -> Text::OptColors {
                    if (connect_peer_option_available(game)) {
                        return {};
                    } else {
                        return FontColors{ColorConstant::med_blue_gray,
                                          ColorConstant::rich_black};
                    }
                }());

            settings_text_->assign(locale_string(LocaleString::menu_settings));
            save_and_quit_text_->assign(
                locale_string(LocaleString::menu_save_and_quit));

            draw_cursor(pfrm);
        }

        pfrm.screen().fade(smoothstep(0.f, fade_duration, fade_timer_));
    } else {

        const auto& line = [&]() -> Text& {
            switch (cursor_loc_) {
            default:
            case 0:
                return *resume_text_;
            case 1:
                return *settings_text_;
            case 2:
                return *connect_peer_text_;
            case 3:
                return *save_and_quit_text_;
            }
        }();
        const Float y_center = pfrm.screen().size().y / 2;
        const Float y_line = line.coord().y * 8;
        const auto y_diff = (y_line - y_center) * 0.4f;

        y_offset_ = interpolate(Float(y_diff), y_offset_, delta * 0.00001f);

        pfrm.set_overlay_origin(0, y_offset_);


        anim_timer_ += delta;
        if (anim_timer_ > milliseconds(75)) {
            anim_timer_ = 0;
            if (++anim_index_ > 1) {
                anim_index_ = 0;
            }
            draw_cursor(pfrm);
        }

        if (pfrm.keyboard().down_transition<Key::down>() and cursor_loc_ < 3) {
            cursor_loc_ += 1;
            draw_cursor(pfrm);
        } else if (pfrm.keyboard().down_transition<Key::up>() and
                   cursor_loc_ > 0) {
            cursor_loc_ -= 1;
            draw_cursor(pfrm);
        } else if (pfrm.keyboard().down_transition<Key::action_2>()) {
            switch (cursor_loc_) {
            case 0:
                return state_pool_.create<ActiveState>(game);
            case 1:
                return state_pool_.create<EditSettingsState>();
            case 2:
                if (connect_peer_option_available(game)) {
                    return state_pool_.create<NetworkConnectSetupState>();
                }
                break;
            case 3:
                return state_pool_.create<GoodbyeState>();
            }
        } else if (pfrm.keyboard().down_transition<Key::start>()) {
            cursor_loc_ = 0;
            return state_pool_.create<ActiveState>(game);
        }
    }

    if (pfrm.keyboard().pressed<Key::select>()) {
        log_timer_ += delta;
        if (log_timer_ > seconds(1)) {
            return state_pool_.create<LogfileViewerState>();
        }
    } else {
        log_timer_ = 0;
    }

    return null_state();
}


void PauseScreenState::exit(Platform& pfrm, Game& game, State& next_state)
{
    pfrm.set_overlay_origin(0, 0);

    resume_text_.reset();
    connect_peer_text_.reset();
    settings_text_.reset();
    save_and_quit_text_.reset();

    pfrm.fill_overlay(0);

    if (dynamic_cast<ActiveState*>(&next_state)) {
        pfrm.screen().fade(0.f);
    }
}


/////////////////////////////////////////////////////////////////////////////////
// NewLevelIdleState
////////////////////////////////////////////////////////////////////////////////


void NewLevelIdleState::receive(const net_event::NewLevelIdle&,
                                Platform& pfrm,
                                Game&)
{
    info(pfrm, "got new level idle msg");
    peer_ready_ = true;
}


void NewLevelIdleState::receive(const net_event::NewLevelSyncSeed& sync_seed,
                                Platform& pfrm,
                                Game&)
{
    info(pfrm, "received seed value");
    if (not pfrm.network_peer().is_host()) {
        rng::critical_state = sync_seed.random_state_.get();
        ready_ = true;
    }
}


void NewLevelIdleState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    if (pfrm.network_peer().is_connected()) {

        const auto str =
            locale_string(LocaleString::level_transition_awaiting_peers);

        const auto margin = centered_text_margins(pfrm, str_len(str));

        auto screen_tiles = calc_screen_tiles(pfrm);

        text_.emplace(pfrm,
                      OverlayCoord{(u8)margin, (u8)(screen_tiles.y / 2 - 1)});

        text_->assign(str);
    }
}


void NewLevelIdleState::exit(Platform& pfrm, Game& game, State& next_state)
{
    text_.reset();
}


StatePtr
NewLevelIdleState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    // Synchronization procedure for seed values at level transition:
    //
    // Players transmit NewLevelIdle messages until both players are ready. Once
    // the host receives a NewLevelIdle message, it should transmit its seed to
    // the other peer. Once the other peer receives the seed, it can continue to
    // the next level.

    if (pfrm.network_peer().is_connected()) {
        timer_ += delta;
        if (timer_ > seconds(1)) {
            info(pfrm, "transmit new level idle msg");
            timer_ -= seconds(1);

            net_event::NewLevelIdle idle_msg;
            idle_msg.header_.message_type_ = net_event::Header::new_level_idle;
            pfrm.network_peer().send_message(
                {(byte*)&idle_msg, sizeof idle_msg});
        }
        if (peer_ready_ and pfrm.network_peer().is_host()) {
            net_event::NewLevelSyncSeed sync_seed;
            sync_seed.header_.message_type_ =
                net_event::Header::new_level_sync_seed;
            sync_seed.random_state_.set(rng::critical_state);
            info(pfrm, "sent seed to peer");
            ready_ = pfrm.network_peer().send_message(
                {(byte*)&sync_seed, sizeof sync_seed});
        }

        net_event::poll_messages(pfrm, game, *this);

    } else {
        ready_ = true;
    }

    if (ready_) {
        Level next_level = game.level() + 1;

        // backdoor for debugging purposes.
        if (pfrm.keyboard().all_pressed<Key::alt_1, Key::alt_2, Key::start>()) {
            return state_pool_.create<CommandCodeState>();
        }


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

        auto zone = zone_info(next_level);
        auto last_zone = zone_info(game.level());

        if (not bosses_remaining and not(zone == last_zone)) {
            pfrm.sleep(120);
            return state_pool_.create<EndingCreditsState>();
        }

        return state_pool_.create<NewLevelState>(next_level);
    }

    return null_state();
}


/////////////////////////////////////////////////////////////////////////////////
// NetworkConnectSetupState
////////////////////////////////////////////////////////////////////////////////


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
            return state_pool_.create<NetworkConnectWaitState>();
            break;

        case Platform::NetworkPeer::internet: {
            // TODO: display some more interesting graphics, allow user to enter
            // ip address
            return state_pool_.create<NetworkConnectWaitState>();
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


/////////////////////////////////////////////////////////////////////////////////
// NetworkConnectWaitState
////////////////////////////////////////////////////////////////////////////////


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
    pfrm.load_overlay_texture("overlay");

    if (not dynamic_cast<MenuState*>(&next_state)) {
        pfrm.screen().fade(0.f);

        if (auto os = dynamic_cast<OverworldState*>(&next_state)) {
            if (pfrm.network_peer().is_connected()) {
                push_notification(
                    pfrm, os, locale_string(LocaleString::peer_connected));
            } else {
                push_notification(
                    pfrm,
                    os,
                    locale_string(LocaleString::peer_connection_failed));
            }
        }
    }
}


StatePtr
NetworkConnectWaitState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    switch (pfrm.network_peer().interface()) {
    case Platform::NetworkPeer::serial_cable: {

        const char* str = " Waiting for Peer...";

        const auto margin = centered_text_margins(pfrm, utf8::len(str));

        Text t(pfrm, {static_cast<u8>(margin), 2});

        t.assign(str);
        pfrm.network_peer().listen();
        return state_pool_.create<ActiveState>(game);
    }

    case Platform::NetworkPeer::internet: {
        // TODO: display some more interesting graphics, allow user to enter ip
        // address, display our ip address if we're the host.
        if (pfrm.keyboard().down_transition<Key::action_1>()) {
            pfrm.network_peer().connect("127.0.0.1");
            return state_pool_.create<PauseScreenState>(false);
        } else if (pfrm.keyboard().down_transition<Key::action_2>()) {
            pfrm.network_peer().listen();

            net_event::SyncSeed s;
            s.random_state_.set(rng::critical_state);
            net_event::transmit(pfrm, s);

            return state_pool_.create<PauseScreenState>(false);
        }
        break;
    }
    }

    return null_state();
}


/////////////////////////////////////////////////////////////////////////////////
// SignalJammerSelectorState
////////////////////////////////////////////////////////////////////////////////


void SignalJammerSelectorState::print(Platform& pfrm, const char* str)
{
    const auto margin = centered_text_margins(pfrm, str_len(str));

    // const auto s_tiles = calc_screen_tiles(pfrm);

    text_.emplace(pfrm, OverlayCoord{});

    left_text_margin(*text_, margin);
    text_->append(str);
    right_text_margin(*text_, margin);
}


void SignalJammerSelectorState::enter(Platform& pfrm,
                                      Game& game,
                                      State& prev_state)
{
    cached_camera_ = game.camera();

    const auto player_pos = game.player().get_position();
    const auto ssize = pfrm.screen().size();
    game.camera().set_position(
        pfrm, {player_pos.x - ssize.x / 2, player_pos.y - ssize.y / 2});

    game.camera().set_speed(1.4f);

    print(pfrm, " ");
}


void SignalJammerSelectorState::exit(Platform& pfrm, Game& game, State&)
{
    game.effects().get<Reticule>().clear();
    game.effects().get<Proxy>().clear();
    game.camera() = cached_camera_;
    text_.reset();
}


StatePtr
SignalJammerSelectorState::update(Platform& pfrm, Game& game, Microseconds dt)
{
    MenuState::update(pfrm, game, dt);

    timer_ += dt;

    constexpr auto fade_duration = milliseconds(500);
    const auto target_fade = 0.75f;

    if (target_) {
        game.camera().push_ballast(game.player().get_position());
        game.camera().update(pfrm, dt, target_->get_position());
    }

    for (auto& rt : game.effects().get<Reticule>()) {
        rt->update(pfrm, game, dt);
    }

    for (auto& p : game.effects().get<Proxy>()) {
        p->update(pfrm, game, dt);
    }

    switch (mode_) {
    case Mode::fade_in: {
        const auto amount =
            1.f - target_fade * smoothstep(0.f, fade_duration, timer_);

        if (amount > (1.f - target_fade)) {
            pfrm.screen().fade(amount);
        }

        if (timer_ > fade_duration) {
            pfrm.screen().fade(
                1.f - target_fade, ColorConstant::rich_black, {}, false);
            timer_ = 0;
            if ((target_ = make_selector_target(game))) {
                mode_ = Mode::update_selector;
                selector_start_pos_ = game.player().get_position();
                game.effects().spawn<Reticule>(selector_start_pos_);

                print(pfrm, locale_string(LocaleString::select_target_text));

            } else {
                return state_pool_.create<InventoryState>(true);
            }
        }
        break;
    }

    case Mode::selected: {
        if (timer_ > milliseconds(100)) {
            timer_ = 0;

            if (flicker_anim_index_ > 9) {
                target_->make_allied(true);

                // Typically, we would have consumed the item already, in the
                // inventory item handler, but in some cases, we want to exit
                // out of this state without using the item.
                consume_selected_item(game);

                for (auto& p : game.effects().get<Proxy>()) {
                    p->colorize({ColorConstant::null, 0});
                }

                pfrm.sleep(8);

                return state_pool_.create<InventoryState>(true);

            } else {
                if (flicker_anim_index_++ % 2 == 0) {
                    for (auto& p : game.effects().get<Proxy>()) {
                        p->colorize({ColorConstant::green, 180});
                    }
                } else {
                    for (auto& p : game.effects().get<Proxy>()) {
                        p->colorize({ColorConstant::silver_white, 255});
                    }
                }
            }
        }
        break;
    }

    case Mode::active: {
        if (pfrm.keyboard().down_transition<Key::action_2>()) {
            if (target_) {
                mode_ = Mode::selected;
                timer_ = 0;
                pfrm.sleep(3);
            }
        } else if (pfrm.keyboard().down_transition<Key::action_1>()) {
            return state_pool_.create<InventoryState>(true);
        }

        if (pfrm.keyboard().down_transition<Key::right>() or
            pfrm.keyboard().down_transition<Key::left>()) {
            selector_index_ += 1;
            timer_ = 0;
            if ((target_ = make_selector_target(game))) {
                mode_ = Mode::update_selector;
                selector_start_pos_ = [&] {
                    for (auto& sel : game.effects().get<Reticule>()) {
                        return sel->get_position();
                    }
                    return game.player().get_position();
                }();
            }
        }
        break;
    }

    case Mode::update_selector: {

        constexpr auto anim_duration = milliseconds(150);

        if (timer_ <= anim_duration) {
            const auto pos = interpolate(target_->get_position(),
                                         selector_start_pos_,
                                         Float(timer_) / anim_duration);

            for (auto& sel : game.effects().get<Reticule>()) {
                sel->move(pos);
            }
        } else {
            for (auto& sel : game.effects().get<Reticule>()) {
                sel->move(target_->get_position());
            }
            for (auto& p : game.effects().get<Proxy>()) {
                p->pulse(ColorConstant::green);
            }
            mode_ = Mode::active;
            timer_ = 0;
        }
        break;
    }
    }


    return null_state();
}


Enemy* SignalJammerSelectorState::make_selector_target(Game& game)
{
    constexpr u32 count = 15;

    // I wish it wasn't necessary to hold a proxy buffer locally, oh well...
    Buffer<Proxy, count> proxies;
    Buffer<Enemy*, count> targets;
    game.enemies().transform([&](auto& buf) {
        for (auto& element : buf) {
            using T = typename std::remove_reference<decltype(buf)>::type;

            using VT = typename T::ValueType::element_type;

            if (distance(element->get_position(),
                         game.player().get_position()) < 128) {
                if constexpr (std::is_base_of<Enemy, VT>()
                              // NOTE: Currently, I am not allowing hacking for
                              // enemies that do physical damage. On the Gameboy
                              // Advance, it simply isn't realistic to do
                              // collision checking between each enemy, and
                              // every other type of enemy. You also cannot turn
                              // bosses into allies.
                              and not std::is_same<VT, Drone>() and
                              not std::is_same<VT, SnakeBody>() and
                              not std::is_same<VT, SnakeTail>() and
                              not std::is_same<VT, SnakeHead>() and
                              not std::is_same<VT, Gatekeeper>() and
                              not std::is_same<VT, TheFirstExplorer>()) {
                    targets.push_back(element.get());
                    proxies.emplace_back(*element.get());
                }
            }
        }
    });

    if (not targets.empty()) {
        if (selector_index_ >= static_cast<int>(targets.size())) {
            selector_index_ %= targets.size();
        }
        if (targets[selector_index_] not_eq target_) {
            game.effects().get<Proxy>().clear();
            game.effects().spawn<Proxy>(proxies[selector_index_]);
        }
        return targets[selector_index_];
    } else {
        return nullptr;
    }
}


/////////////////////////////////////////////////////////////////////////////////
// GoodbyeState
////////////////////////////////////////////////////////////////////////////////


void GoodbyeState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.write_save_data(&game.persistent_data(),
                         sizeof game.persistent_data());

    pfrm.speaker().stop_music();

    const auto s_tiles = calc_screen_tiles(pfrm);
    text_.emplace(pfrm, OverlayCoord{1, u8(s_tiles.y - 2)});
    text_->append(locale_string(LocaleString::goodbye_text));
}


StatePtr GoodbyeState::update(Platform& pfrm, Game&, Microseconds delta)
{
    wait_timer_ += delta;
    if (wait_timer_ > seconds(6)) {
        pfrm.soft_exit();
        pfrm.sleep(10);
    }

    return null_state();
}
