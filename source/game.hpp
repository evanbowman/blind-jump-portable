#pragma once

#include <algorithm>

#include "camera.hpp"
#include "entity/bosses/firstExplorer.hpp"
#include "entity/details/item.hpp"
#include "entity/details/itemChest.hpp"
#include "entity/details/rubble.hpp"
#include "entity/details/transporter.hpp"
#include "entity/effects/explosion.hpp"
#include "entity/effects/laser.hpp"
#include "entity/effects/orbshot.hpp"
#include "entity/enemies/critter.hpp"
#include "entity/enemies/dasher.hpp"
#include "entity/enemies/drone.hpp"
#include "entity/enemies/probe.hpp"
#include "entity/enemies/scarecrow.hpp"
#include "entity/enemies/snake.hpp"
#include "entity/enemies/turret.hpp"
#include "entity/entityGroup.hpp"
#include "entity/player.hpp"
#include "function.hpp"
#include "graphics/overlay.hpp"
#include "platform/platform.hpp"
#include "state.hpp"


class Game {
public:
    Game(Platform& platform);

    void update(Platform& platform, Microseconds delta);

    void render(Platform& platform);

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
                                   FirstExplorer>;

    using DetailGroup = EntityGroup<30, ItemChest, Item, Rubble>;
    using EffectGroup = EntityGroup<20, OrbShot, Laser, Explosion>;
    // using BossGroup = EntityGroup<1, FirstExplorer>;

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
        return persistent_data_.score_;
    }

    auto& inventory()
    {
        return persistent_data_.inventory_;
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

    void stop_music(Platform& pfrm);
    void play_music(Platform& pfrm, const char* track_name, Microseconds track_length);

private:
    TileMap tiles_;
    Camera camera_;
    Player player_;
    EnemyGroup enemies_;
    DetailGroup details_;
    EffectGroup effects_;
    // BossGroup bosses_;
    Transporter transporter_;
    PersistentData persistent_data_;
    StatePtr state_;

    const char* music_track_name_;
    Microseconds music_track_length_;
    Microseconds music_remaining_;

    Buffer<std::pair<DeferredCallback, Microseconds>, 10> deferred_callbacks_;

    void seed_map(Platform& platform, TileMap& workspace);
    void regenerate_map(Platform& platform);
    bool respawn_entities(Platform& platform);

    void update_transitions(Platform& pf, Microseconds dt);
};


bool is_boss_level(Level level);
