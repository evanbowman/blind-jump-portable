#pragma once

#include "camera.hpp"
#include "enemies/critter.hpp"
#include "enemies/dasher.hpp"
#include "enemies/probe.hpp"
#include "enemies/turret.hpp"
#include "details/item.hpp"
#include "details/itemChest.hpp"
#include "details/transporter.hpp"
#include "platform.hpp"
#include "player.hpp"
#include "transformGroup.hpp"


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

    inline Player& get_player()
    {
        return player_;
    }

    inline TileMap& get_tiles()
    {
        return tiles_;
    }

    using EnemyGroup = EntityGroup<Turret, Dasher, Probe>;
    using DetailGroup = EntityGroup<ItemChest>;
    using EffectGroup = EntityGroup<Item>;

    inline EffectGroup& get_effects()
    {
        return effects_;
    }

    inline DetailGroup& get_details()
    {
        return details_;
    }

    inline EnemyGroup& get_enemies()
    {
        return enemies_;
    }

private:
    Buffer<const Sprite*, Platform::Screen::sprite_limit> display_buffer;
    s32 level_;
    u32 update_counter_;
    TileMap tiles_;
    Camera camera_;
    Player player_;
    EnemyGroup enemies_;
    DetailGroup details_;
    EffectGroup effects_;
    Transporter transporter_;
    Microseconds counter_;
    SaveData save_data_;

    enum class State { active, fade_out, fade_in } state_;

    void next_level(Platform& platform);
    void regenerate_map(Platform& platform);
    bool respawn_entities(Platform& platform);

    void update_transitions(Platform& pf, Microseconds dt);
};
