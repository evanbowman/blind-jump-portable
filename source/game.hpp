#pragma once

#include "camera.hpp"
#include "entity/enemies/critter.hpp"
#include "entity/enemies/dasher.hpp"
#include "entity/enemies/probe.hpp"
#include "entity/enemies/turret.hpp"
#include "entity/details/item.hpp"
#include "entity/details/itemChest.hpp"
#include "entity/details/transporter.hpp"
#include "entity/player.hpp"
#include "platform/platform.hpp"
#include "transformGroup.hpp"
#include "state.hpp"


template <typename Arg>
using EntityBuffer = Buffer<EntityRef<Arg>, Arg::spawn_limit()>;


template <typename... Args>
class EntityGroup : public TransformGroup<EntityBuffer<Args>...> {
public:
    template <typename T> auto& get()
    {
        return TransformGroup<EntityBuffer<Args>...>::template get<
            EntityBuffer<T>>();
    }
};


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

    using EnemyGroup = EntityGroup<Turret, Dasher, Probe>;
    using DetailGroup = EntityGroup<ItemChest>;
    using EffectGroup = EntityGroup<Item>;

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

    void next_level(Platform& platform);

private:
    s32 level_;
    TileMap tiles_;
    Camera camera_;
    Player player_;
    EnemyGroup enemies_;
    DetailGroup details_;
    EffectGroup effects_;
    Transporter transporter_;
    Microseconds counter_;
    SaveData save_data_;
    State* state_;

    void regenerate_map(Platform& platform);
    bool respawn_entities(Platform& platform);

    void update_transitions(Platform& pf, Microseconds dt);
};
