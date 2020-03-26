#pragma once

#include <algorithm>

#include "camera.hpp"
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
#include "entity/enemies/snake.hpp"
#include "entity/enemies/turret.hpp"
#include "entity/entityGroup.hpp"
#include "entity/player.hpp"
#include "function.hpp"
#include "platform/platform.hpp"
#include "state.hpp"
#include "graphics/overlay.hpp"


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

    using EnemyGroup =
        EntityGroup<20, Drone, Turret, Dasher, SnakeHead, SnakeBody, SnakeTail>;

    using DetailGroup = EntityGroup<30, ItemChest, Item, Rubble>;
    using EffectGroup = EntityGroup<20, OrbShot, Laser, Explosion>;

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

    Score& score()
    {
        return persistent_data_.score_;
    }

    auto& inventory()
    {
        return persistent_data_.inventory_;
    }

    // NOTE: May need to increase internal storage for Function eventually... not sure...
    using DeferredCallback = Function<16, void(Platform&, Game&)>;

    bool on_timeout(Microseconds expire_time, const DeferredCallback& callback)
    {
        return deferred_callbacks_.push_back({callback, expire_time});
    }

private:
    TileMap tiles_;
    Camera camera_;
    Player player_;
    EnemyGroup enemies_;
    DetailGroup details_;
    EffectGroup effects_;
    Transporter transporter_;
    PersistentData persistent_data_;
    StatePtr state_;

    Buffer<std::pair<DeferredCallback, Microseconds>, 10> deferred_callbacks_;

    void regenerate_map(Platform& platform);
    bool respawn_entities(Platform& platform);

    void update_transitions(Platform& pf, Microseconds dt);
};
