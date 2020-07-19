#pragma once

#include <algorithm>

#include "camera.hpp"
#include "entity/bosses/gatekeeper.hpp"
#include "entity/bosses/theFirstExplorer.hpp"
#include "entity/details/item.hpp"
#include "entity/details/itemChest.hpp"
#include "entity/details/rubble.hpp"
#include "entity/details/scavenger.hpp"
#include "entity/details/transporter.hpp"
#include "entity/effects/explosion.hpp"
#include "entity/effects/firstExplorerBigLaser.hpp"
#include "entity/effects/firstExplorerSmallLaser.hpp"
#include "entity/effects/laser.hpp"
#include "entity/effects/orbshot.hpp"
#include "entity/enemies/dasher.hpp"
#include "entity/enemies/drone.hpp"
#include "entity/enemies/scarecrow.hpp"
#include "entity/enemies/snake.hpp"
#include "entity/enemies/theif.hpp"
#include "entity/enemies/sinkhole.hpp"
#include "entity/enemies/turret.hpp"
#include "entity/entityGroup.hpp"
#include "entity/player.hpp"
#include "function.hpp"
#include "localization.hpp"
#include "persistentData.hpp"
#include "platform/platform.hpp"
#include "state.hpp"
#include "powerup.hpp"


class Game {
public:
    Game(Platform& platform);

    void update(Platform& platform, Microseconds delta);

    void render(Platform& platform);

    using Powerups = Buffer<Powerup, Powerup::max_>;
    inline Powerups& powerups()
    {
        return powerups_;
    }

    inline Player& player()
    {
        return player_;
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
                                   TheFirstExplorer,
                                   Gatekeeper,
                                   GatekeeperShield,
                                   Theif,
                                   Sinkhole>;

    using DetailGroup = EntityGroup<30, ItemChest, Item, Rubble, Scavenger>;
    using EffectGroup = EntityGroup<20,
                                    OrbShot,
                                    Laser,
                                    Explosion,
                                    FirstExplorerBigLaser,
                                    FirstExplorerSmallLaser>;

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

    void next_level(Platform& platform, std::optional<Level> set_level = {});

    Level level() const
    {
        return persistent_data_.level_;
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

    bool on_timeout(Microseconds expire_time, const DeferredCallback& callback)
    {
        return deferred_callbacks_.push_back({callback, expire_time});
    }

    PersistentData& persistent_data()
    {
        return persistent_data_;
    }

private:
    bool load_save_data(Platform& pfrm);

    TileMap tiles_;
    Camera camera_;
    Player player_;
    EnemyGroup enemies_;
    DetailGroup details_;
    EffectGroup effects_;
    // BossGroup bosses_;
    Transporter transporter_;
    Score score_;
    Inventory inventory_;
    PersistentData persistent_data_;
    StatePtr state_;
    Powerups powerups_;

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
    ColorConstant injury_glow_color_;

    BackgroundGenerator generate_background_;
    DecorationGenerator generate_decoration_;
};


const ZoneInfo& zone_info(Level level);
const ZoneInfo& current_zone(Game& game);

bool operator==(const ZoneInfo&, const ZoneInfo&);


Game::DeferredCallback screen_flash_animation(int remaining);


enum {
    boss_0_level = 10,
    boss_1_level = 21,
    boss_2_level = 32,
};


void animate_starfield(Platform& pfrm, Microseconds delta);


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
