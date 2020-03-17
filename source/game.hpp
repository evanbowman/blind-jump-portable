#pragma once

#include <algorithm>

#include "camera.hpp"
#include "entity/details/item.hpp"
#include "entity/details/itemChest.hpp"
#include "entity/details/transporter.hpp"
#include "entity/effects/laser.hpp"
#include "entity/effects/orbshot.hpp"
#include "entity/enemies/critter.hpp"
#include "entity/enemies/dasher.hpp"
#include "entity/enemies/probe.hpp"
#include "entity/enemies/snake.hpp"
#include "entity/enemies/turret.hpp"
#include "entity/entityGroup.hpp"
#include "entity/player.hpp"
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

    using EnemyGroup =
        EntityGroup<20, Turret, Dasher, Probe, SnakeHead, SnakeBody, SnakeTail>;

    using DetailGroup = EntityGroup<20, ItemChest, Item>;
    using EffectGroup = EntityGroup<20, OrbShot, Laser>;

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
        return save_data_.level_;
    }

    Score& score()
    {
        return save_data_.score_;
    }


private:
    TileMap tiles_;
    Camera camera_;
    Player player_;
    EnemyGroup enemies_;
    DetailGroup details_;
    EffectGroup effects_;
    Transporter transporter_;
    SaveData save_data_;
    State* state_;

    void regenerate_map(Platform& platform);
    bool respawn_entities(Platform& platform);

    void update_transitions(Platform& pf, Microseconds dt);
};
