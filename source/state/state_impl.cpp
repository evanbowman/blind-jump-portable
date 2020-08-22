#include "state_impl.hpp"
#include "bitvector.hpp"
#include "conf.hpp"
#include "number/random.hpp"


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


Bitmatrix<TileMap::width, TileMap::height> visited;


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


void repaint_health_score(Platform& pfrm,
                          Game& game,
                          std::optional<UIMetric>* health,
                          std::optional<UIMetric>* score,
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
                      Buffer<UIMetric, Powerup::max_>* powerups,
                      UIMetric::Align align)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    if (clean) {

        repaint_health_score(pfrm, game, health, score, align);

        powerups->clear();

        u8 write_pos = screen_tiles.y - 2;

        for (auto& powerup : game.powerups()) {
            powerups->emplace_back(
                pfrm,
                OverlayCoord{(*health)->position().x, write_pos},
                powerup.icon_index(),
                powerup.parameter_,
                align);

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


void update_powerups(Platform& pfrm,
                     Game& game,
                     std::optional<UIMetric>* health,
                     std::optional<UIMetric>* score,
                     Buffer<UIMetric, Powerup::max_>* powerups,
                     UIMetric::Align align)
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
        repaint_powerups(
            pfrm, game, update_all, health, score, powerups, align);
    }
}


void update_ui_metrics(Platform& pfrm,
                       Game& game,
                       Microseconds delta,
                       std::optional<UIMetric>* health,
                       std::optional<UIMetric>* score,
                       Buffer<UIMetric, Powerup::max_>* powerups,
                       Entity::Health last_health,
                       Score last_score,
                       UIMetric::Align align)
{
    update_powerups(pfrm, game, health, score, powerups, align);

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


// FIXME!!! This does decrease the size of the compiled ROM though...
#include "overworldState.cpp"
#include "bossDeathSequenceState.cpp"
#include "activeState.cpp"
#include "fadeInState.cpp"
#include "warpInState.cpp"
#include "preFadePauseState.cpp"
#include "glowFadeState.cpp"
#include "fadeOutState.cpp"
#include "deathFadeState.cpp"
#include "respawnWaitState.cpp"
#include "deathContinueState.cpp"
#include "inventoryState.cpp"
#include "quickSelectInventoryState.cpp"
#include "notebookState.cpp"
#include "imageViewState.cpp"
#include "mapSystemState.cpp"
#include "quickMapState.cpp"
#include "newLevelState.cpp"
#include "introCreditsState.cpp"
#include "launchCutsceneState.cpp"
#include "editSettingsState.cpp"
#include "logfileViewerState.cpp"
#include "commandCodeState.cpp"
#include "endingCreditsState.cpp"
#include "pauseScreenState.cpp"
#include "newLevelIdleState.cpp"
#include "networkConnectSetupState.cpp"
#include "networkConnectWaitState.cpp"
#include "signalJammerState.cpp"
#include "goodbyeState.cpp"



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
         return state_pool().create<ImageViewState>(str,
                                                   ColorConstant::steel_blue);
     },
     LocaleString::old_poster_title},
    {Item::Type::postal_advert, item_icon(Item::Type::old_poster_1),
     [](Platform&, Game&) {
         static const auto str = "postal_advert_flattened";
         return state_pool().create<ImageViewState>(str,
                                                    ColorConstant::steel_blue);
     },
     LocaleString::postal_advert_title},
    {STANDARD_ITEM_HANDLER(surveyor_logbook),
     [](Platform&, Game&) {
         return state_pool().create<NotebookState>(
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
     [](Platform&, Game&) { return state_pool().create<MapSystemState>(); },
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
         return state_pool().create<ImageViewState>(str,
                                                   ColorConstant::steel_blue);
     },
     LocaleString::seed_packet_title},
    {STANDARD_ITEM_HANDLER(engineer_notebook),
     [](Platform&, Game&) {
         return state_pool().create<NotebookState>(
             locale_string(LocaleString::engineer_notebook_str));
     },
     LocaleString::engineer_notebook_title},
    {STANDARD_ITEM_HANDLER(signal_jammer),
     [](Platform&, Game&) {
         return state_pool().create<SignalJammerSelectorState>();
         return null_state();
     },
     LocaleString::signal_jammer_title,
     InventoryItemHandler::custom},
    {STANDARD_ITEM_HANDLER(navigation_pamphlet),
     [](Platform&, Game&) {
         return state_pool().create<NotebookState>(
             locale_string(LocaleString::navigation_pamphlet));
     },
     LocaleString::navigation_pamphlet_title},
    {STANDARD_ITEM_HANDLER(orange),
     [](Platform& pfrm, Game& game) {
         game.player().add_health(1);
         if (game.inventory().item_count(Item::Type::orange_seeds) == 0) {
             game.inventory().push_item(pfrm, game, Item::Type::orange_seeds);
         }
         return null_state();
     },
     LocaleString::orange_title,
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
    if (not is_walkable__fast(
            game.tiles().get_tile(player_tile.x, player_tile.y))) {
        // Player movement isn't constrained to tiles exactly, and sometimes the
        // player's map icon displays as inside of a wall.
        if (is_walkable__fast(
                game.tiles().get_tile(player_tile.x + 1, player_tile.y))) {
            player_tile.x += 1;
        } else if (is_walkable__fast(game.tiles().get_tile(
                       player_tile.x, player_tile.y + 1))) {
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
                  int y_skip_top,
                  int y_skip_bot,
                  bool force_icons,
                  PathBuffer* path)
{
    auto set_tile = [&](s8 x, s8 y, int icon, bool dodge = true) {
        if (y < y_skip_top or y >= TileMap::height - y_skip_bot) {
            return;
        }
        const auto tile = pfrm.get_tile(Layer::overlay, x + x_start, y);
        if (dodge and (tile == 133 or tile == 132)) {
            // ...
        } else {
            pfrm.set_tile(Layer::overlay, x + x_start, y, icon);
        }
    };

    auto render_map_icon = [&](Entity& entity, s16 icon) {
        auto t = to_tile_coord(entity.get_position().cast<s32>());
        if (is_walkable__fast(game.tiles().get_tile(t.x, t.y))) {
            set_tile(t.x, t.y, icon);
        }
    };

    const auto current_column = std::min(
        TileMap::width,
        interpolate(TileMap::width, (decltype(TileMap::width))0, percentage));
    game.tiles().for_each([&](Tile t, s8 x, s8 y) {
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
                if (visited.get(x2, y2 + 1)) {
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

        const auto player_tile = get_constrained_player_tile_coord(game);

        set_tile(player_tile.x, player_tile.y, 142);

        return true;
    }

    return false;
}


StatePtr State::initial()
{
    return state_pool().create<IntroLegalMessage>();
}


[[noreturn]] void factory_reset(Platform& pfrm)
{
    PersistentData data;
    data.magic_ = 0xBAD;
    pfrm.write_save_data(&data, sizeof data);
    pfrm.fatal();
}
