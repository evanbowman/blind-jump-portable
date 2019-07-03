#pragma once

#include "platform.hpp"
#include "transformGroup.hpp"
#include "critter.hpp"
#include "dasher.hpp"
#include "player.hpp"
#include "transientEffect.hpp"
#include "itemChest.hpp"
#include "camera.hpp"


template <typename Arg>
using EntityBuffer = Buffer<Arg*, Arg::spawn_limit()>;


template <typename ...Args>
using EntityGroup = TransformGroup<EntityBuffer<Args>...>;


class Game {
public:
    Game();

    void update(Platform& platform, Microseconds delta);

    void render(Platform& platform);

private:
    Buffer<const Sprite*, Screen::sprite_limit> display_buffer;
    Camera camera_;
    Player* player_;
    EntityGroup<Critter, Dasher> enemies_;
    EntityGroup<ItemChest> details_;
    EntityGroup<> effects_;
};
