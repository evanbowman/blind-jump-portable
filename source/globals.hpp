#pragma once

#include <variant>

#include "blind_jump/game.hpp"


struct BlindJumpGlobalData {
    Game::EnemyGroup::Pool_ enemy_pool_;
    Game::EnemyGroup::NodePool_ enemy_node_pool_;

    Game::DetailGroup::Pool_ detail_pool_;
    Game::DetailGroup::NodePool_ detail_node_pool_;

    Game::EffectGroup::Pool_ effect_pool_;
    Game::EffectGroup::NodePool_ effect_node_pool_;

    Bitmatrix<TileMap::width, TileMap::height> visited_;
};


struct ExampleGlobalData {
    u8 buffer[2048];
};


// This data used to be static members of the EntityGroup<> class template. But
// I am trying to keep better track of global variables across the project, so I
// am declaring them all in this one structure. I have been toying with the idea
// of compiling multiple games into a single rom/executable, which makes keeping
// track of global variable memory usage important.
using Globals = std::variant<BlindJumpGlobalData,
                             ExampleGlobalData>;


Globals& globals();
