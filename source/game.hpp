#pragma once

#include <algorithm>

#include "memory/pool.hpp"
#include "camera.hpp"
#include "entity/enemies/critter.hpp"
#include "entity/enemies/dasher.hpp"
#include "entity/enemies/probe.hpp"
#include "entity/enemies/turret.hpp"
#include "entity/enemies/snake.hpp"
#include "entity/details/item.hpp"
#include "entity/details/itemChest.hpp"
#include "entity/details/transporter.hpp"
#include "entity/effects/orbshot.hpp"
#include "entity/player.hpp"
#include "platform/platform.hpp"
#include "transformGroup.hpp"
#include "state.hpp"
#include "list.hpp"


using EntityNode = BiNode<EntityRef<Entity>>;


template <u32 Capacity>
using EntityNodePool = Pool<sizeof(EntityNode), Capacity, alignof(EntityNode)>;


template <typename Arg, u32 Capacity>
using EntityBuffer = List<EntityRef<Arg>, EntityNodePool<Capacity>>;


template <size_t Capacity, typename... Members>
class EntityGroup : public TransformGroup<EntityBuffer<Members, Capacity>...> {
public:

    EntityGroup() : TransformGroup<EntityBuffer<Members, Capacity>...>(node_pool_)
    {
    }

    template <typename T, typename... CtorArgs>
    bool spawn(CtorArgs&&... ctorArgs)
    {
        auto deleter = [](T* obj) {
                obj->~T();
                pool_.post(reinterpret_cast<byte*>(obj));
            };

        if (auto mem = pool_.get()) {
            new (mem) T(std::forward<CtorArgs>(ctorArgs)...);

            this->get<T>().push({reinterpret_cast<T*>(mem), deleter});

            return true;
        } else {
            return false;
        }
    }

    template <typename T> auto& get()
    {
        return TransformGroup<EntityBuffer<Members, Capacity>...>::template get<
            EntityBuffer<T, Capacity>>();
    }

private:
    using Pool_ = Pool<std::max({sizeof(Members)...}),
                       Capacity,
                       std::max({alignof(Members)...})>;

    using NodePool_ = EntityNodePool<Capacity>;
    static NodePool_ node_pool_;

    static Pool_ pool_;
};


template <size_t Cap, typename... Members>
typename EntityGroup<Cap, Members...>::Pool_ EntityGroup<Cap, Members...>::pool_;

template <size_t Cap, typename... Members>
typename EntityGroup<Cap, Members...>::NodePool_ EntityGroup<Cap, Members...>::node_pool_;



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

    using EnemyGroup = EntityGroup<10, Turret, Dasher, Probe, Snake>;
    using DetailGroup = EntityGroup<20, ItemChest, Item>;
    using EffectGroup = EntityGroup<10, OrbShot>;

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
