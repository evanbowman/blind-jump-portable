#include "script/lisp.hpp"
#include "state_impl.hpp"


void OverworldState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    // pfrm.enable_feature("vignette", true);
}


void OverworldState::exit(Platform& pfrm, Game&, State& next_state)
{
    notification_text.reset();
    fps_text_.reset();
    network_tx_msg_text_.reset();
    network_rx_msg_text_.reset();
    network_tx_loss_text_.reset();
    network_rx_loss_text_.reset();
    link_saturation_text_.reset();
    scratch_buf_avail_text_.reset();

    // In case we're in the middle of an entry/exit animation for the
    // notification bar.
    for (int i = 0; i < 32; ++i) {
        pfrm.set_tile(Layer::overlay, i, 0, 0);
    }
    if (locale_requires_doublesize_font()) {
        for (int i = 0; i < 32; ++i) {
            pfrm.set_tile(Layer::overlay, i, 1, 0);
        }
    }

    time_remaining_text_.reset();
    time_remaining_icon_.reset();

    notification_status = NotificationStatus::hidden;
}


StringBuffer<32> format_time(u32 seconds, bool include_hours)
{
    StringBuffer<32> result;
    char buffer[32];

    int hours = seconds / 3600;
    int remainder = (int)seconds - hours * 3600;
    int mins = remainder / 60;
    remainder = remainder - mins * 60;
    int secs = remainder;

    if (include_hours) {
        locale_num2str(hours, buffer, 10);
        result += buffer;
        result += ":";
    }

    locale_num2str(mins, buffer, 10);
    result += buffer;
    result += ":";

    if (secs < 10) {
        result += "0";
    }

    locale_num2str(secs, buffer, 10);
    result += buffer;

    return result;
}


void OverworldState::display_time_remaining(Platform& pfrm, Game& game)
{

    const auto current_secs =
        (int)game.persistent_data().speedrun_clock_.whole_seconds();

    const auto fmt = format_time(current_secs);

    const auto screen_tiles = calc_screen_tiles(pfrm);

    const u8 text_x_pos = (screen_tiles.x - utf8::len(fmt.c_str())) - 1;

    const u8 text_y_pos = screen_tiles.y - 2;


    time_remaining_text_.emplace(pfrm, OverlayCoord{text_x_pos, text_y_pos});

    time_remaining_text_->assign(fmt.c_str());

    time_remaining_icon_.emplace(
        pfrm, 422, OverlayCoord{u8(text_x_pos - 1), text_y_pos});
}


void OverworldState::receive(const net_event::QuickChat& chat,
                             Platform& pfrm,
                             Game& game)
{
    NotificationStr str;
    str += locale_string(pfrm, LocaleString::chat_chat)->c_str();

    str += locale_string(pfrm, static_cast<LocaleString>(chat.message_.get()))
               ->c_str();

    push_notification(pfrm, game.state(), str);
}


void OverworldState::receive(const net_event::PlayerHealthChanged& event,
                             Platform& pfrm,
                             Game& game)
{
    const auto health = event.new_health_.get();

    if (health > 3) {
        // No need to annoy the player with notifications when the multiplayer
        // peer's health is not yet critically low.
        return;
    }

    NotificationStr str;
    str += locale_string(pfrm, LocaleString::peer_health_changed)->c_str();

    char buffer[20];
    locale_num2str(health, buffer, 10);

    str += buffer;

    push_notification(pfrm, game.state(), str);
}


void OverworldState::receive(const net_event::NewLevelIdle& n,
                             Platform& pfrm,
                             Game& game)
{
    if (++idle_rx_count_ == 120) {
        idle_rx_count_ = 0;

        push_notification(
            pfrm,
            game.state(),
            locale_string(pfrm, LocaleString::peer_transport_waiting)->c_str());
    }
}


void OverworldState::receive(const net_event::ItemChestShared& s,
                             Platform& pfrm,
                             Game& game)
{
    bool id_collision = false;
    game.details().transform([&](auto& buf) {
        for (auto& e : buf) {
            if (e->id() == s.id_.get()) {
                id_collision = true;
            }
        }
    });
    if (id_collision) {
        error(pfrm, "failed to receive shared item chest, ID collision!");
        return;
    }

    if (game.peer() and
        create_item_chest(game, game.peer()->get_position(), s.item_, false)) {
        pfrm.speaker().play_sound("dropitem", 3);
        (*game.details().get<ItemChest>().begin())->override_id(s.id_.get());
    } else {
        error(pfrm, "failed to allocate shared item chest");
    }
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
    bool done = false;

    game.enemies().transform([&](auto& buf) {
        if (done) {
            return;
        }
        for (auto& e : buf) {
            if (e->id() == s.id_.get()) {
                e->sync(s, game);
                done = true;
                return;
            }
        }
    });
}


void OverworldState::receive(const net_event::PlayerInfo& p,
                             Platform& pfrm,
                             Game& game)
{
    if (not game.peer()) {
        game.peer().emplace(pfrm);
    }
    game.peer()->sync(pfrm, game, p);
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
    info.opt1_ = 0;
    info.opt2_ = 0;
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
    //
    // Note: These numbers seem to result in smooth enough performance on a
    // physical GBA, so I will not increase the refresh rates per now. Three
    // updates per second is fairly smooth with interpolation.
    static const auto player_refresh_rate = seconds(1) / 20;

    static auto update_counter = player_refresh_rate;

    update_counter -= delta;
    if (update_counter <= 0) {
        update_counter = player_refresh_rate;

        if (game.player().get_health() > 0) {
            transmit_player_info(pfrm, game);
        }

        if (pfrm.network_peer().is_host()) {
            net_event::SyncSeed s;
            s.random_state_.set(rng::critical_state);
            net_event::transmit(pfrm, s);
        }
    }


    net_event::poll_messages(pfrm, game, *this);

    if (game.peer()) {
        game.peer()->update(pfrm, game, delta);
    }
}


void player_death(Platform& pfrm, Game& game, const Vec2<Float>& position)
{
    pfrm.speaker().play_sound("explosion1", 3, position);
    big_explosion(pfrm, game, position);
}


void CommonNetworkListener::receive(const net_event::PlayerDied&,
                                    Platform& pfrm,
                                    Game& game)
{
    if (game.player().get_health() == 0) {
        // If we're already dead, and the peer just died, then no-one can revive
        // us. Time to disconnect.
        safe_disconnect(pfrm);
    } else {
        if (game.peer()) {
            const auto peer_pos = game.peer()->get_position();

            player_death(pfrm, game, peer_pos);

            game.details().spawn<Signpost>(peer_pos,
                                           Signpost::Type::knocked_out_peer);

            game.peer().reset();
        }
    }
}


void OverworldState::show_stats(Platform& pfrm, Game& game, Microseconds delta)
{
    fps_timer_ += delta;
    fps_frame_count_ += 1;

    if (fps_timer_ >= seconds(1)) {
        fps_timer_ -= seconds(1);

        fps_text_.emplace(pfrm, OverlayCoord{1, 2});
        link_saturation_text_.emplace(pfrm, OverlayCoord{1, 3});
        network_tx_msg_text_.emplace(pfrm, OverlayCoord{1, 4});
        network_rx_msg_text_.emplace(pfrm, OverlayCoord{1, 5});
        network_tx_loss_text_.emplace(pfrm, OverlayCoord{1, 6});
        network_rx_loss_text_.emplace(pfrm, OverlayCoord{1, 7});
        scratch_buf_avail_text_.emplace(pfrm, OverlayCoord{1, 8});

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
        network_tx_msg_text_->append(" tx");
        network_rx_msg_text_->append(net_stats.receive_count_);
        network_rx_msg_text_->append(" rx");
        network_tx_loss_text_->append(net_stats.transmit_loss_, tx_loss_colors);
        network_tx_loss_text_->append(" tl", tx_loss_colors);
        network_rx_loss_text_->append(net_stats.receive_loss_, rx_loss_colors);
        network_rx_loss_text_->append(" rl", rx_loss_colors);
        link_saturation_text_->append(net_stats.link_saturation_);
        link_saturation_text_->append(" lnsat");
        scratch_buf_avail_text_->append(pfrm.scratch_buffers_remaining());
        scratch_buf_avail_text_->append(" sbr");
    }
}


StatePtr OverworldState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    animate_starfield(pfrm, delta);

    const auto prior_sec =
        game.persistent_data().speedrun_clock_.whole_seconds();
    game.persistent_data().speedrun_clock_.count_up(delta);
    const auto current_sec =
        game.persistent_data().speedrun_clock_.whole_seconds();

    if (game.persistent_data().settings_.show_speedrun_clock_) {
        if (not time_remaining_text_ or prior_sec not_eq current_sec) {
            display_time_remaining(pfrm, game);
        }
    }

    if (pfrm.network_peer().is_connected()) {
        multiplayer_sync(pfrm, game, delta);
    } else if (game.peer()) {
        player_death(pfrm, game, game.peer()->get_position());
        game.peer().reset();
        push_notification(
            pfrm,
            game.state(),
            locale_string(pfrm, LocaleString::peer_lost)->c_str());
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
    if (get_powerup(game, Powerup::Type::lethargy)) {
        enemy_timestep /= 2;
    }

    if (pfrm.keyboard().up_transition(game.action1_key())) {
        camera_snap_timer_ = seconds(2) + milliseconds(250);
    }

    if (camera_snap_timer_ > 0) {
        if (game.persistent_data().settings_.button_mode_ ==
            Settings::ButtonMode::strafe_combined) {

            if (pfrm.keyboard().down_transition(game.action2_key())) {
                camera_snap_timer_ = 0;
            }
        }
        camera_snap_timer_ -= delta;
    }

    const bool boss_level = is_boss_level(game.level());

    auto bosses_remaining = [&] {
        return boss_level and (length(game.enemies().get<Wanderer>()) or
                               length(game.enemies().get<Gatekeeper>()) or
                               length(game.enemies().get<Twin>()) or
                               length(game.enemies().get<InfestedCore>()));
    };

    const auto [boss_position, boss_defeated_text] =
        [&]() -> std::pair<Vec2<Float>, LocaleString> {
        if (boss_level) {
            Vec2<Float> result;
            LocaleString lstr = LocaleString::empty;
            for (auto& elem : game.enemies().get<Wanderer>()) {
                result = elem->get_position();
                lstr = elem->defeated_text();
            }
            for (auto& elem : game.enemies().get<Gatekeeper>()) {
                result = elem->get_position();
                lstr = elem->defeated_text();
            }
            for (auto& elem : game.enemies().get<Twin>()) {
                result = elem->get_position();
                lstr = elem->defeated_text();
            }
            for (auto& elem : game.enemies().get<InfestedCore>()) {
                result = elem->get_position();
                lstr = elem->defeated_text();
            }
            return {result, lstr};
        } else {
            return {{}, LocaleString::empty};
        }
    }();

    const bool bosses_were_remaining = bosses_remaining();

    bool enemies_remaining = false;
    bool enemies_destroyed = false;
    bool enemies_visible = false;
    game.enemies().transform([&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                (*it)->on_death(pfrm, game);
                it = entity_buf.erase(it);
                game.rumble(pfrm, milliseconds(150));
                enemies_destroyed = true;
            } else {
                enemies_remaining = true;

                (*it)->update(pfrm, game, enemy_timestep);

                if (camera_tracking_ and
                    (pfrm.keyboard().pressed(game.action1_key()) or
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
        str += locale_string(pfrm, LocaleString::level_clear)->c_str();

        push_notification(pfrm, game.state(), str);

        if (not pfrm.network_peer().is_connected()) {
            lisp::dostring(
                pfrm.load_file_contents("scripts", "waypoint_clear.lisp"));
        }
    }


    switch (notification_status) {
    case NotificationStatus::hidden:
        break;

    case NotificationStatus::flash: {
        const bool bigfont = locale_requires_doublesize_font();

        if (bigfont) {
            for (int x = 0; x < 32; ++x) {
                pfrm.set_tile(Layer::overlay, x, 0, 110);
            }
            for (int x = 0; x < 32; ++x) {
                pfrm.set_tile(Layer::overlay, x, 1, 110);
            }
        } else {
            for (int x = 0; x < 32; ++x) {
                pfrm.set_tile(Layer::overlay, x, 0, 108);
            }
        }
        notification_status = NotificationStatus::flash_animate;
        notification_text_timer = -1 * milliseconds(5);
        break;
    }

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

            const bool bigfont = locale_requires_doublesize_font();

            FontConfiguration font_conf;
            font_conf.double_size_ = bigfont;

            notification_text.emplace(pfrm, OverlayCoord{0, 0}, font_conf);

            auto margin = centered_text_margins(
                pfrm, utf8::len(notification_str.c_str()) * (bigfont ? 2 : 1));

            left_text_margin(*notification_text, margin / (bigfont ? 2 : 1));
            notification_text->append(notification_str.c_str());
            right_text_margin(*notification_text, margin / (bigfont ? 2 : 1));

            notification_status = NotificationStatus::display;
        }
        break;

    case NotificationStatus::display:
        if (notification_text_timer > 0) {
            notification_text_timer -= delta;

        } else {
            notification_text_timer = 0;

            if (locale_requires_doublesize_font()) {
                notification_status = NotificationStatus::exit_row2;

                for (int x = 0; x < 32; ++x) {
                    pfrm.set_tile(Layer::overlay, x, 1, 112);
                }
            } else {
                notification_status = NotificationStatus::exit;
            }


            for (int x = 0; x < 32; ++x) {
                pfrm.set_tile(Layer::overlay, x, 0, 112);
            }
        }
        break;

    case NotificationStatus::exit_row2: {
        notification_text_timer += delta;
        if (notification_text_timer > (locale_requires_doublesize_font()
                                           ? milliseconds(17)
                                           : milliseconds(34))) {
            notification_text_timer = 0;

            const auto tile = pfrm.get_tile(Layer::overlay, 0, 1);
            if (tile < 120) {
                for (int x = 0; x < 32; ++x) {
                    pfrm.set_tile(Layer::overlay, x, 1, tile + 1);
                }
            } else {
                notification_text_timer = 0;
                notification_status = NotificationStatus::exit;
            }
        }
        break;
    }

    case NotificationStatus::exit: {
        notification_text_timer += delta;
        if (notification_text_timer > (locale_requires_doublesize_font()
                                           ? milliseconds(17)
                                           : milliseconds(34))) {
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

    game.camera().update(pfrm,
                         camera_mode_override_
                             ? *camera_mode_override_
                             : game.persistent_data().settings_.camera_mode_,
                         delta,
                         player.get_position());


    check_collisions(pfrm, game, player, game.details().get<Item>());
    check_collisions(pfrm, game, player, game.effects().get<OrbShot>());
    check_collisions(
        pfrm, game, player, game.effects().get<ConglomerateShot>());

    if (UNLIKELY(boss_level)) {
        check_collisions(
            pfrm, game, player, game.effects().get<WandererBigLaser>());
        check_collisions(
            pfrm, game, player, game.effects().get<WandererSmallLaser>());
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
                      not std::is_same<Sinkhole, VT>() and
                      not std::is_same<InfestedCore, VT>()) {
            check_collisions(pfrm, game, player, buf);
        }

        if constexpr (not std::is_same<Sinkhole, VT>()) {
            check_collisions(pfrm, game, game.effects().get<Laser>(), buf);
        }
    });

    if (bosses_were_remaining and not bosses_remaining()) {
        game.effects().transform([](auto& buf) { buf.clear(); });
        pfrm.sleep(10);
        return state_pool().create<BossDeathSequenceState>(
            game, boss_position, boss_defeated_text);
    }

    return null_state();
}
