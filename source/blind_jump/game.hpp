#pragma once

#include <algorithm>

#include "camera.hpp"
#include "entity/bosses/gatekeeper.hpp"
#include "entity/bosses/infestedCore.hpp"
#include "entity/bosses/theTwins.hpp"
#include "entity/bosses/wanderer.hpp"
#include "entity/details/cutsceneBird.hpp"
#include "entity/details/cutsceneCloud.hpp"
#include "entity/details/debris.hpp"
#include "entity/details/item.hpp"
#include "entity/details/itemChest.hpp"
#include "entity/details/lander.hpp"
#include "entity/details/rubble.hpp"
#include "entity/details/scavenger.hpp"
#include "entity/details/signpost.hpp"
#include "entity/details/transporter.hpp"
#include "entity/effects/conglomerateShot.hpp"
#include "entity/effects/dialogBubble.hpp"
#include "entity/effects/dynamicEffect.hpp"
#include "entity/effects/explosion.hpp"
#include "entity/effects/laser.hpp"
#include "entity/effects/orbshot.hpp"
#include "entity/effects/particle.hpp"
#include "entity/effects/proxy.hpp"
#include "entity/effects/reticule.hpp"
#include "entity/effects/staticEffect.hpp"
#include "entity/effects/uiNumber.hpp"
#include "entity/effects/wandererBigLaser.hpp"
#include "entity/effects/wandererSmallLaser.hpp"
#include "entity/enemies/compactor.hpp"
#include "entity/enemies/dasher.hpp"
#include "entity/enemies/drone.hpp"
#include "entity/enemies/golem.hpp"
#include "entity/enemies/scarecrow.hpp"
#include "entity/enemies/sinkhole.hpp"
#include "entity/enemies/snake.hpp"
#include "entity/enemies/theif.hpp"
#include "entity/enemies/turret.hpp"
#include "entity/entityGroup.hpp"
#include "entity/peerPlayer.hpp"
#include "entity/player.hpp"
#include "function.hpp"
#include "localeString.hpp"
#include "persistentData.hpp"
#include "platform/platform.hpp"
#include "powerup.hpp"
#include "rumble.hpp"
#include "state.hpp"


class Game {
public:
    Game(Platform& platform);

    void update(Platform& platform, Microseconds delta);

    void render(Platform& platform);

    inline Powerups& powerups()
    {
        return powerups_;
    }

    inline Player& player()
    {
        return player_;
    }

    inline std::optional<PeerPlayer>& peer()
    {
        return peer_player_;
    }

    inline std::optional<Scavenger>& scavenger()
    {
        return scavenger_;
    }

    inline TileMap& tiles()
    {
        return tiles_;
    }

    using EnemyGroup = EntityGroup<20,
                                   Drone,
                                   Turret,
                                   Dasher,
                                   SnakeHead,
                                   SnakeBody,
                                   SnakeTail,
                                   Scarecrow,
                                   Wanderer,
                                   Gatekeeper,
                                   GatekeeperShield,
                                   Sinkhole,
                                   Twin,
                                   Compactor,
                                   InfestedCore,
                                   Golem>;

    using DetailGroup = EntityGroup<30,
                                    ItemChest,
                                    Item,
                                    Rubble,
                                    Lander,
                                    CutsceneCloud,
                                    CutsceneBird,
                                    Debris,
                                    Signpost>;

    using EffectGroup = EntityGroup<20,
                                    UINumber,
                                    Reticule,
                                    Proxy,
                                    OrbShot,
                                    AlliedOrbShot,
                                    Laser,
                                    PeerLaser,
                                    Explosion,
                                    WandererBigLaser,
                                    WandererSmallLaser,
                                    ConglomerateShot,
                                    Particle,
                                    DialogBubble,
                                    StaticEffect,
                                    DynamicEffect>;

    inline Transporter& transporter()
    {
        return transporter_;
    }

    inline EffectGroup& effects()
    {
        return effects_;
    }

    inline DetailGroup& details()
    {
        return details_;
    }

    inline EnemyGroup& enemies()
    {
        return enemies_;
    }

    inline Camera& camera()
    {
        return camera_;
    }

    void rumble(Platform& pfrm, Microseconds duration);

    void next_level(Platform& platform, std::optional<Level> set_level = {});

    Level level() const
    {
        return persistent_data_.level_.get();
    }

    auto& highscores()
    {
        return persistent_data_.highscores_;
    }

    Score& score()
    {
        return score_;
    }

    auto& inventory()
    {
        return inventory_;
    }

    State* state()
    {
        return state_.get();
    }

    // NOTE: May need to increase internal storage for Function eventually... not sure...
    using DeferredCallback = Function<16, void(Platform&, Game&)>;

    bool on_timeout(Platform& pfrm,
                    Microseconds expire_time,
                    const DeferredCallback& callback)
    {
        if (not deferred_callbacks_.push_back({callback, expire_time})) {
            warning(pfrm, "failed to enq timeout");
            return false;
        }
        return true;
    }

    PersistentData& persistent_data()
    {
        return persistent_data_;
    }

    Settings::Difficulty difficulty() const
    {
        return persistent_data_.settings_.difficulty_;
    }

    Key action1_key() const
    {
        return persistent_data_.settings_.action1_key_;
    }

    Key action2_key() const
    {
        return persistent_data_.settings_.action2_key_;
    }

    u16 get_boss_target() const
    {
        return boss_target_;
    }

    void set_boss_target(u16 target)
    {
        boss_target_ = target;
    }

private:
    bool load_save_data(Platform& pfrm);

    void init_script(Platform& pfrm);

    TileMap tiles_;
    Camera camera_;
    Player player_;
    EnemyGroup enemies_;
    DetailGroup details_;
    EffectGroup effects_;
    Transporter transporter_;
    Score score_;
    Inventory inventory_;
    PersistentData persistent_data_;
    StatePtr next_state_;
    StatePtr state_;
    Powerups powerups_;
    Rumble rumble_;

    u16 boss_target_;

    std::optional<PeerPlayer> peer_player_;
    std::optional<Scavenger> scavenger_;

    Buffer<std::pair<DeferredCallback, Microseconds>, 10> deferred_callbacks_;

    void seed_map(Platform& platform, TileMap& workspace);
    void regenerate_map(Platform& platform);
    bool respawn_entities(Platform& platform);

    void update_transitions(Platform& pf, Microseconds dt);
};


bool is_boss_level(Level level);


using BackgroundGenerator = void (*)(Platform&, Game&);
using DecorationGenerator = int (*)(int x, int y, const TileMap&);


struct ZoneInfo {
    LocaleString title_line_1;
    LocaleString title_line_2;
    const char* spritesheet_name_;
    const char* tileset0_name_;
    const char* tileset1_name_;
    const char* music_name_;
    Microseconds music_offset_;

    ColorConstant energy_glow_color_;
    ColorConstant energy_glow_color_2_;
    ColorConstant injury_glow_color_;
    ColorConstant laser_flicker_bright_color_;

    BackgroundGenerator generate_background_;
    DecorationGenerator generate_decoration_;
};


const ZoneInfo& zone_info(Level level);
const ZoneInfo& current_zone(Game& game);

bool operator==(const ZoneInfo&, const ZoneInfo&);


enum {
    boss_0_level = 8,
    boss_1_level = 19,
    boss_2_level = 28,
    boss_3_level = 36,
    boss_max_level
};


void animate_starfield(Platform& pfrm, Microseconds delta);


using SimulatedMiles = int;
SimulatedMiles distance_travelled(Level level);


template <typename T>
Float apply_gravity_well(const Vec2<Float>& center_pos,
                         T& target,
                         int fast_radius,
                         Float radius,
                         Float strength,
                         Float falloff,
                         bool pull = true)
{
    const auto& target_pos = target.get_position();

    if (manhattan_length(target_pos, center_pos) < fast_radius) {

        const Float dist = distance(target_pos, center_pos);

        if (dist < radius) {

            const auto dir = [&] {
                if (pull) {
                    return direction(target_pos, center_pos);
                } else {
                    return direction(center_pos, target_pos);
                }
            }();

            if (falloff not_eq 0.f) {
                target.apply_force(interpolate(0.f, strength, dist / falloff) *
                                   dir);
            } else {
                target.apply_force(strength * dir);
            }

            return dist;
        }
    }

    return 1000.f;
}


inline int enemies_remaining(Game& game)
{
    int remaining = 0;

    game.enemies().transform([&](auto& buf) {
        using T = typename std::remove_reference<decltype(buf)>::type;

        using VT = typename T::ValueType::element_type;

        // SnakeBody and SnakeTail are technically not
        // enemies, we only want to count each snake head
        // segment.
        if (not std::is_same<VT, SnakeBody>() and
            not std::is_same<VT, SnakeTail>()) {

            remaining += length(buf);
        }
    });

    return remaining;
}


// Create an item chest, evicting any less-essential entities from the detail
// group in the process. Mainly intended for item sharing in multiplayer
// mode.
inline bool create_item_chest(Game& game,
                              const Vec2<Float>& position,
                              Item::Type item,
                              bool locked)
{
    // If you wanted to prevent one player from spawning too many item chests
    // and lagging the game:
    //
    // if (length(game.details().get<ItemChest>()) > 6) {
    //     for (auto it = game.details().get<ItemChest>().begin();
    //          it not_eq game.details().get<ItemChest>().end(); ++it) {
    //         if ((*it)->state() == ItemChest::State::opened) {
    //             game.details().get<ItemChest>().erase(it);
    //             break;
    //         }
    //     }
    // }
    //
    if (game.details().spawn<ItemChest>(position, item, locked)) {
        return true;
    }

    for (auto it = game.details().get<ItemChest>().begin();
         it not_eq game.details().get<ItemChest>().end();
         ++it) {
        if ((*it)->state() == ItemChest::State::opened) {
            game.details().get<ItemChest>().erase(it);
            return game.details().spawn<ItemChest>(position, item, locked);
        }
    }

    if (not game.details().get<Rubble>().empty()) {
        game.details().get<Rubble>().pop();
        return game.details().spawn<ItemChest>(position, item, locked);
    }

    return false;
}


bool share_item(Platform& pfrm,
                Game& game,
                const Vec2<Float>& position,
                Item::Type item);


int base_price(Item::Type item);


using LevelRange = std::array<Level, 2>;
LevelRange level_range(Item::Type item);

bool level_in_range(Level level, LevelRange range);


void newgame(Platform& pfrm, Game& game);


void safe_disconnect(Platform& pfrm);


Entity* get_entity_by_id(Game& game, Entity::Id id);
