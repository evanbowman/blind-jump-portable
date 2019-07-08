#pragma once

#include "camera.hpp"
#include "critter.hpp"
#include "dasher.hpp"
#include "itemChest.hpp"
#include "platform.hpp"
#include "player.hpp"
#include "transformGroup.hpp"
#include "transporter.hpp"


template <typename Arg>
using EntityBuffer = Buffer<Arg, Arg::element_type::spawn_limit()>;


template <typename... Args>
using EntityGroup = TransformGroup<EntityBuffer<EntityRef<Args>>...>;


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

    using EnemyGroup = EntityGroup<Critter, Dasher>;
    using DetailGroup = EntityGroup<ItemChest>;
    using EffectGroup = EntityGroup<>;

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
    Buffer<const Sprite*, Screen::sprite_limit> display_buffer;
    s32 level_;
    TileMap tiles_;
    Camera camera_;
    Player player_;
    EnemyGroup enemies_;
    DetailGroup details_;
    EffectGroup effects_;
    Transporter transporter_;

    void next_level(Platform& platform);
    void regenerate_map(Platform& platform);
    void respawn_entities(Platform& platform);
};
