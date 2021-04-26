#include "state_impl.hpp"
#include "bitvector.hpp"
#include "globals.hpp"
#include "number/random.hpp"
#include "script/lisp.hpp"


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


static void lethargy_update(Platform& pfrm, Game& game)
{
    if (auto lethargy = get_powerup(game, Powerup::Type::lethargy)) {
        lethargy->dirty_ = true;
        if (lethargy->parameter_.get() > 0) {
            lethargy->parameter_.set(lethargy->parameter_.get() - seconds(1));
            if (not game.on_timeout(pfrm, seconds(1), lethargy_update)) {
                // Because, if we fail to enqueue the timeout, I believe that
                // the powerup would get stuck.
                lethargy->parameter_.set(0);
            }
        } else {
            lethargy->parameter_.set(0);
        }
    }
}


static void add_lethargy_powerup(Platform& pfrm, Game& game)
{
    add_powerup(game,
                Powerup::Type::lethargy,
                seconds(18),
                Powerup::DisplayMode::timestamp);

    // For simplicity, I've implemented lethargy with timeouts. Easier to keep
    // behavior consistent across multiplayer games this way. Otherwise, each
    // game state would need to remember to update the powerup countdown timer
    // on each update step.
    game.on_timeout(pfrm, seconds(1), [](Platform& pfrm, Game& game) {
        lethargy_update(pfrm, game);
    });
}


void CommonNetworkListener::receive(const net_event::LethargyActivated&,
                                    Platform& pfrm,
                                    Game& game)
{
    add_lethargy_powerup(pfrm, game);
    push_notification(
        pfrm,
        game.state(),
        locale_string(pfrm, LocaleString::peer_used_lethargy)->c_str());
}


void CommonNetworkListener::receive(const net_event::ProgramVersion& vn,
                                    Platform& pfrm,
                                    Game& game)
{
    const auto major = vn.info_.major_.get();
    const auto minor = vn.info_.minor_.get();
    const auto subminor = vn.info_.subminor_.get();
    const auto revision = vn.info_.revision_.get();

    auto local_vn = std::make_tuple(PROGRAM_MAJOR_VERSION,
                                    PROGRAM_MINOR_VERSION,
                                    PROGRAM_SUBMINOR_VERSION,
                                    PROGRAM_VERSION_REVISION);

    auto peer_vn = std::tie(major, minor, subminor, revision);

    if (peer_vn not_eq local_vn) {

        game.peer().reset();

        if (peer_vn > local_vn) {

            push_notification(
                pfrm,
                game.state(),
                locale_string(pfrm, LocaleString::update_required)->c_str());
        } else {
            auto str = locale_string(pfrm, LocaleString::peer_requires_update);
            push_notification(pfrm, game.state(), str->c_str());
        }

        safe_disconnect(pfrm);

    } else {
        info(pfrm, "received valid program version");
    }
}


void CommonNetworkListener::receive(const net_event::PlayerEnteredGate&,
                                    Platform& pfrm,
                                    Game& game)
{
    if (game.peer()) {
        game.peer()->warping() = true;
    }

    push_notification(
        pfrm,
        game.state(),
        locale_string(pfrm, LocaleString::peer_transport_waiting)->c_str());
}


void CommonNetworkListener::receive(const net_event::Disconnect&,
                                    Platform& pfrm,
                                    Game& game)
{
    pfrm.network_peer().disconnect();
}


bool within_view_frustum(const Platform::Screen& screen,
                         const Vec2<Float>& pos);


void show_boss_health(Platform& pfrm, Game& game, int bar, Float percentage)
{
    if (bar >= ActiveState::boss_health_bar_count) {
        return;
    }
    if (auto state = dynamic_cast<ActiveState*>(game.state())) {
        if (not state->boss_health_bar_[bar]) {
            state->boss_health_bar_[bar].emplace(
                pfrm,
                6,
                OverlayCoord{u8(pfrm.screen().size().x / 8 - (2 + bar)), 1});
        }

        state->boss_health_bar_[bar]->set_health(percentage);
    }
}


void hide_boss_health(Game& game)
{
    if (auto state = dynamic_cast<ActiveState*>(game.state())) {
        for (auto& bar : state->boss_health_bar_) {
            bar.reset();
        }
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


void repaint_health_score(Platform& pfrm,
                          Game& game,
                          std::optional<UIMetric>* health,
                          std::optional<UIMetric>* score,
                          std::optional<MediumIcon>* dodge,
                          UIMetric::Align align)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    const u8 x = align == UIMetric::Align::right ? screen_tiles.x - 2 : 1;

    health->emplace(
        pfrm,
        OverlayCoord{x, u8(screen_tiles.y - (3 + game.powerups().size()))},
        145,
        (int)game.player().get_health(),
        align);

    score->emplace(
        pfrm,
        OverlayCoord{x, u8(screen_tiles.y - (2 + game.powerups().size()))},
        146,
        game.score(),
        align);
}


void repaint_powerups(Platform& pfrm,
                      Game& game,
                      bool clean,
                      std::optional<UIMetric>* health,
                      std::optional<UIMetric>* score,
                      std::optional<MediumIcon>* dodge,
                      Buffer<UIMetric, Powerup::max_>* powerups,
                      UIMetric::Align align)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    auto get_normalized_param = [](Powerup& powerup) {
        switch (powerup.display_mode_) {
        case Powerup::DisplayMode::integer:
            return powerup.parameter_.get();
        case Powerup::DisplayMode::timestamp:
            return powerup.parameter_.get() / 1000000;
        }
        return s32(0);
    };

    if (clean) {

        repaint_health_score(pfrm, game, health, score, dodge, align);

        powerups->clear();

        u8 write_pos = screen_tiles.y - 2;

        for (auto& powerup : game.powerups()) {
            powerups->emplace_back(
                pfrm,
                OverlayCoord{(*health)->position().x, write_pos},
                powerup.icon_index(),
                get_normalized_param(powerup),
                align);

            powerup.dirty_ = false;

            --write_pos;
        }
    } else {
        for (u32 i = 0; i < game.powerups().size(); ++i) {
            if (game.powerups()[i].dirty_) {
                const auto p = get_normalized_param(game.powerups()[i]);
                (*powerups)[i].set_value(p);
                game.powerups()[i].dirty_ = false;
            }
        }
    }
}


void update_powerups(Platform& pfrm,
                     Game& game,
                     std::optional<UIMetric>* health,
                     std::optional<UIMetric>* score,
                     std::optional<MediumIcon>* dodge,
                     Buffer<UIMetric, Powerup::max_>* powerups,
                     UIMetric::Align align)
{
    bool update_powerups = false;
    bool update_all = false;
    for (auto it = game.powerups().begin(); it not_eq game.powerups().end();) {
        if (it->parameter_.get() <= 0) {
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

    if (game.powerups().size() not_eq powerups->size()) {
        update_powerups = true;
        update_all = true;
    }

    if (update_powerups) {
        repaint_powerups(
            pfrm, game, update_all, health, score, dodge, powerups, align);
    }
}


void update_ui_metrics(Platform& pfrm,
                       Game& game,
                       Microseconds delta,
                       std::optional<UIMetric>* health,
                       std::optional<UIMetric>* score,
                       std::optional<MediumIcon>* dodge,
                       Buffer<UIMetric, Powerup::max_>* powerups,
                       Entity::Health last_health,
                       Score last_score,
                       bool dodge_was_ready,
                       UIMetric::Align align)
{
    update_powerups(pfrm, game, health, score, dodge, powerups, align);

    const auto screen_tiles = calc_screen_tiles(pfrm);

    if (dodge and
        static_cast<bool>(game.player().dodges()) not_eq dodge_was_ready) {
        dodge->emplace(
            pfrm,
            game.player().dodges() ? 383 : 387,
            OverlayCoord{1, u8(screen_tiles.y - (5 + game.powerups().size()))});
    }

    if (dodge and static_cast<bool>(game.player().dodges()) == 0) {
        if (pfrm.keyboard().pressed(game.action1_key())) {
            dodge->emplace(
                pfrm,
                379,
                OverlayCoord{
                    1, u8(screen_tiles.y - (5 + game.powerups().size()))});
        }
        if (pfrm.keyboard().up_transition(game.action1_key())) {
            dodge->emplace(
                pfrm,
                387,
                OverlayCoord{
                    1, u8(screen_tiles.y - (5 + game.powerups().size()))});
        }
    }

    if (last_health not_eq game.player().get_health()) {
        net_event::PlayerHealthChanged hc;
        hc.new_health_.set(game.player().get_health());

        net_event::transmit(pfrm, hc);

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


// FIXME: this shouldn't be global...
std::optional<Platform::Keyboard::RestoreState> restore_keystates;


int PauseScreenState::cursor_loc_ = 0;


static StatePoolInst state_pool_;


StatePtr null_state()
{
    return {nullptr, state_deleter};
}


void state_deleter(State* s)
{
    if (s) {
        s->~State();
        state_pool_.pool_.post(reinterpret_cast<byte*>(s));
    }
}


StatePoolInst& state_pool()
{
    return state_pool_;
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
     [](Platform& pfrm, Game&) {
         StringBuffer<48> seed_packet_texture = "old_poster_";
         seed_packet_texture += locale_language_name(locale_get_language());
         seed_packet_texture += "_flattened";

         if (not pfrm.overlay_texture_exists(seed_packet_texture.c_str())) {
             seed_packet_texture = "old_poster_flattened";
         }

         return state_pool().create<ImageViewState>(seed_packet_texture.c_str(),
                                                    ColorConstant::steel_blue);
     },
     LocaleString::old_poster_title},
    {Item::Type::postal_advert,
     item_icon(Item::Type::old_poster_1),
     [](Platform&, Game&) {
         // static const auto str = "postal_advert_flattened";
         // return state_pool().create<ImageViewState>(str,
         //                                            ColorConstant::steel_blue);
         return null_state();
     },
     LocaleString::postal_advert_title},
    {Item::Type::long_jump_z2,
     item_icon(Item::Type::long_jump_z2),
     [](Platform& pfrm, Game& game) {
         const auto c = current_zone(game).energy_glow_color_;
         pfrm.speaker().play_sound("bell", 5);
         game.persistent_data().level_.set(boss_0_level);
         game.score() = 0; // Otherwise, people could exploit the jump packs to
                           // keep replaying a zone, in order to get a really
                           // high score.
         return state_pool().create<PreFadePauseState>(game, c);
     },
     LocaleString::long_jump_z2_title,
     {LocaleString::sc_dialog_jumpdrive, LocaleString::empty},
     InventoryItemHandler::yes},
    {Item::Type::long_jump_z3,
     item_icon(Item::Type::long_jump_z2),
     [](Platform& pfrm, Game& game) {
         const auto c = current_zone(game).energy_glow_color_;
         pfrm.speaker().play_sound("bell", 5);
         game.persistent_data().level_.set(boss_1_level);
         game.score() = 0;
         return state_pool().create<PreFadePauseState>(game, c);
     },
     LocaleString::long_jump_z3_title,
     {LocaleString::sc_dialog_jumpdrive, LocaleString::empty},
     InventoryItemHandler::yes},
    {Item::Type::long_jump_z4,
     item_icon(Item::Type::long_jump_z2),
     [](Platform& pfrm, Game& game) {
         const auto c = current_zone(game).energy_glow_color_;
         pfrm.speaker().play_sound("bell", 5);
         game.persistent_data().level_.set(boss_2_level);
         game.score() = 0;
         return state_pool().create<PreFadePauseState>(game, c);
     },
     LocaleString::long_jump_z4_title,
     {LocaleString::sc_dialog_jumpdrive, LocaleString::empty},
     InventoryItemHandler::yes},
    {Item::Type::long_jump_home,
     item_icon(Item::Type::long_jump_z2),
     [](Platform& pfrm, Game& game) {
         const auto c = current_zone(game).energy_glow_color_;
         pfrm.speaker().play_sound("bell", 5);
         game.persistent_data().level_.set(boss_max_level);
         return state_pool().create<PreFadePauseState>(game, c);
     },
     LocaleString::long_jump_home_title,
     {LocaleString::sc_dialog_jumpdrive, LocaleString::empty},
     InventoryItemHandler::yes},
    {STANDARD_ITEM_HANDLER(worker_notebook_1),
     [](Platform& pfrm, Game&) {
         return state_pool().create<NotebookState>(
             locale_string(pfrm, LocaleString::worker_notebook_1_str));
     },
     LocaleString::worker_notebook_1_title},
    {Item::Type::worker_notebook_2,
     item_icon(Item::Type::worker_notebook_1),
     [](Platform& pfrm, Game&) {
         return state_pool().create<NotebookState>(
             locale_string(pfrm, LocaleString::worker_notebook_2_str));
     },
     LocaleString::worker_notebook_2_title},
    {STANDARD_ITEM_HANDLER(blaster),
     [](Platform&, Game&) { return null_state(); },
     LocaleString::blaster_title},
    {STANDARD_ITEM_HANDLER(accelerator),
     [](Platform&, Game& game) {
         add_powerup(game, Powerup::Type::accelerator, 60);
         return null_state();
     },
     LocaleString::accelerator_title,
     {LocaleString::sc_dialog_accelerator, LocaleString::empty},
     InventoryItemHandler::yes},
    {STANDARD_ITEM_HANDLER(lethargy),
     [](Platform& pfrm, Game& game) {
         add_lethargy_powerup(pfrm, game);
         net_event::LethargyActivated event;
         net_event::transmit(pfrm, event);
         return null_state();
     },
     LocaleString::lethargy_title,
     {LocaleString::sc_dialog_lethargy, LocaleString::empty},
     InventoryItemHandler::yes},
    {STANDARD_ITEM_HANDLER(map_system),
     [](Platform&, Game&) { return state_pool().create<MapSystemState>(); },
     LocaleString::map_system_title,
     {LocaleString::sc_dialog_map_system, LocaleString::empty}},
    {STANDARD_ITEM_HANDLER(explosive_rounds_2),
     [](Platform&, Game& game) {
         add_powerup(game, Powerup::Type::explosive_rounds, 2);
         return null_state();
     },
     LocaleString::explosive_rounds_title,
     {LocaleString::sc_dialog_explosive_rounds, LocaleString::empty},
     InventoryItemHandler::yes},
    {STANDARD_ITEM_HANDLER(seed_packet),
     [](Platform& pfrm, Game&) {
         StringBuffer<48> seed_packet_texture = "seed_packet_";
         seed_packet_texture += locale_language_name(locale_get_language());
         seed_packet_texture += "_flattened";

         if (not pfrm.overlay_texture_exists(seed_packet_texture.c_str())) {
             seed_packet_texture = "seed_packet_flattened";
         }

         return state_pool().create<ImageViewState>(seed_packet_texture.c_str(),
                                                    ColorConstant::steel_blue);
     },
     LocaleString::seed_packet_title},
    {STANDARD_ITEM_HANDLER(engineer_notebook_2),
     [](Platform& pfrm, Game&) {
         return state_pool().create<NotebookState>(
             locale_string(pfrm, LocaleString::engineer_notebook_2_str));
     },
     LocaleString::engineer_notebook_2_title},
    {Item::Type::engineer_notebook_1,
     item_icon(Item::Type::engineer_notebook_2),
     [](Platform& pfrm, Game&) {
         return state_pool().create<NotebookState>(
             locale_string(pfrm, LocaleString::engineer_notebook_1_str));
     },
     LocaleString::engineer_notebook_1_title},
    {STANDARD_ITEM_HANDLER(signal_jammer),
     [](Platform&, Game&) {
         return state_pool().create<SignalJammerSelectorState>();
     },
     LocaleString::signal_jammer_title,
     {LocaleString::sc_dialog_skip, LocaleString::empty},
     InventoryItemHandler::custom},
    {STANDARD_ITEM_HANDLER(navigation_pamphlet),
     [](Platform& pfrm, Game&) {
         return state_pool().create<NotebookState>(
             locale_string(pfrm, LocaleString::navigation_pamphlet));
     },
     LocaleString::navigation_pamphlet_title},
    {STANDARD_ITEM_HANDLER(orange),
     [](Platform& pfrm, Game& game) {
         game.player().heal(pfrm, game, 1);
         if (game.inventory().item_count(Item::Type::orange_seeds) == 0) {
             game.inventory().push_item(pfrm, game, Item::Type::orange_seeds);
         }
         return null_state();
     },
     LocaleString::orange_title,
     {LocaleString::sc_dialog_orange, LocaleString::empty},
     InventoryItemHandler::yes},
    {STANDARD_ITEM_HANDLER(orange_seeds),
     [](Platform&, Game&) { return null_state(); },
     LocaleString::orange_seeds_title}};


const InventoryItemHandler* inventory_item_handler(Item::Type type)
{
    for (auto& handler : inventory_handlers) {
        if (handler.type_ == type) {
            return &handler;
        }
    }
    return nullptr;
}


Vec2<s8> get_constrained_player_tile_coord(Game& game)
{
    auto player_tile = to_tile_coord(game.player().get_position().cast<s32>());
    //u32 integer_text_length(int n);
    if (not is_walkable(game.tiles().get_tile(player_tile.x, player_tile.y))) {
        // Player movement isn't constrained to tiles exactly, and sometimes the
        // player's map icon displays as inside of a wall.
        if (is_walkable(
                game.tiles().get_tile(player_tile.x + 1, player_tile.y))) {
            player_tile.x += 1;
        } else if (is_walkable(game.tiles().get_tile(player_tile.x,
                                                     player_tile.y + 1))) {
            player_tile.y += 1;
        }
    }
    return player_tile;
}


bool draw_minimap(Platform& pfrm,
                  Game& game,
                  Float percentage,
                  int& last_column,
                  int x_start,
                  int y_start,
                  int y_skip_top,
                  int y_skip_bot,
                  bool force_icons,
                  PathBuffer* path)
{
    auto set_tile = [&](s8 x, s8 y, int icon, bool dodge = true) {
        if (y < y_skip_top or y >= TileMap::height - y_skip_bot) {
            return;
        }
        const auto tile =
            pfrm.get_tile(Layer::overlay, x + x_start, y + y_start);
        if (dodge and (tile == 133 or tile == 132)) {
            // ...
        } else {
            pfrm.set_tile(Layer::overlay, x + x_start, y + y_start, icon);
        }
    };

    auto render_map_icon = [&](Entity& entity, s16 icon) {
        auto t = to_tile_coord(entity.get_position().cast<s32>());
        if (is_walkable(game.tiles().get_tile(t.x, t.y))) {
            set_tile(t.x, t.y, icon);
        }
    };

    const auto current_column = std::min(
        TileMap::width,
        interpolate(TileMap::width, (decltype(TileMap::width))0, percentage));
    game.tiles().for_each([&](u8 t, s8 x, s8 y) {
        if (x > current_column or x <= last_column) {
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
                if (std::get<BlindJumpGlobalData>(globals()).visited_.get(x2,
                                                                          y2)) {
                    visited_nearby = true;
                }
            }
        }
        if (not visited_nearby) {
            set_tile(x, y, is_walkable(t) ? 132 : 133, false);
        } else if (is_walkable(t)) {
            set_tile(x, y, 143, false);
        } else {
            if (is_walkable(game.tiles().get_tile(x, y - 1))) {
                set_tile(x, y, 140);
            } else {
                set_tile(x, y, 144, false);
            }
        }
    });
    last_column = current_column;

    if (force_icons or current_column == TileMap::width) {

        if (path) {
            for (u32 i = 0; i < path->size(); ++i) {
                if (i > 0 and i < path->size() - 1) {
                    set_tile((*path)[i].x, (*path)[i].y, 131);
                }
            }
        }

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

        if (game.scavenger()) {
            render_map_icon(*game.scavenger(), 392);
        }

        const auto player_tile = get_constrained_player_tile_coord(game);

        set_tile(player_tile.x, player_tile.y, 142);

        return true;
    }

    return false;
}


StatePtr State::initial(Platform& pfrm, Game& game)
{
    pfrm.keyboard().poll();

    if (game.persistent_data().settings_.initial_lang_selected_ and
        not pfrm.keyboard().pressed<Key::select>()) {
        return state_pool().create<TitleScreenState>();
    } else {
        if (not(lisp::length(lisp::get_var("languages")) == 1)) {
            return state_pool().create<LanguageSelectionState>();
        } else {
            return state_pool().create<TitleScreenState>();
        }
    }
}
