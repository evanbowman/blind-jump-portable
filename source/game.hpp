#pragma once

#include "critter.hpp"
#include "dasher.hpp"
#include "platform.hpp"
#include "player.hpp"
#include "transformGroup.hpp"
// #include "transientEffect.hpp"
#include "camera.hpp"
#include "itemChest.hpp"
#include "transporter.hpp"


template <typename Arg> using EntityBuffer = Buffer<Arg*, Arg::spawn_limit()>;


template <typename... Args>
using EntityGroup = TransformGroup<EntityBuffer<Args>...>;


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
    TileMap tiles_;
    Camera camera_;
    Player player_;
    EnemyGroup enemies_;
    DetailGroup details_;
    EffectGroup effects_;
    Transporter transporter_;

    void regenerate_map(Platform& platform);
};
