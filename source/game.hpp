#pragma once

#include <algorithm>

#include "memory/pool.hpp"
#include "camera.hpp"
#include "entity/enemies/critter.hpp"
#include "entity/enemies/dasher.hpp"
#include "entity/enemies/probe.hpp"
#include "entity/enemies/turret.hpp"
#include "entity/details/item.hpp"
#include "entity/details/itemChest.hpp"
#include "entity/details/transporter.hpp"
#include "entity/effects/projectile.hpp"
#include "entity/player.hpp"
#include "platform/platform.hpp"
#include "transformGroup.hpp"
#include "state.hpp"


template <typename Arg>
using EntityBuffer = Buffer<EntityRef<Arg>, Arg::spawn_limit()>;


template <size_t Capacity, typename... Members>
class EntityGroup : public TransformGroup<EntityBuffer<Members>...> {
public:

    template <typename T, typename... CtorArgs>
    EntityRef<T> spawn(CtorArgs&&... ctorArgs)
    {
        auto deleter = [](T* obj) {
                obj->~T();
                pool_.post(reinterpret_cast<byte*>(obj));
            };

        if (auto mem = pool_.get()) {
            new (mem) T(std::forward<CtorArgs>(ctorArgs)...);
            return {reinterpret_cast<T*>(mem), deleter};
        } else {
            return {nullptr, deleter};
        }
    }

    template <typename T> auto& get()
    {
        return TransformGroup<EntityBuffer<Members>...>::template get<
            EntityBuffer<T>>();
    }

private:
    using Pool_ = Pool<std::max({sizeof(Members)...}),
                       Capacity,
                       std::max({alignof(Members)...})>;

    static Pool_ pool_;
};


template <size_t Cap, typename... Members>
typename EntityGroup<Cap, Members...>::Pool_ EntityGroup<Cap, Members...>::pool_;


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

    using EnemyGroup = EntityGroup<5, Turret, Dasher, Probe>;
    using DetailGroup = EntityGroup<20, ItemChest, Item>;
    using EffectGroup = EntityGroup<10, Projectile>;

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
