#pragma once


#include "entity/entity.hpp"
#include "inventory.hpp"
#include "number/numeric.hpp"


using Level = s32;
using Score = u64;


class Platform;


struct PersistentData {
    static constexpr u32 schema_version = 2;
    static constexpr u32 magic_val = 0xCA55E77E;

    using Identity = u64;
    using Magic = u32;

    Identity id_ = 0;
    Magic magic_ = magic_val;
    u32 seed_ = 11001;
    Level level_ = 0;
    Score score_ = 0;
    Entity::Health player_health_ = 3;

    using HighScores = std::array<Score, 8>;
    HighScores highscores_ = {0};

    Inventory inventory_;

    // Reset level back to zero, initialize various things. Does not overwrite
    // the seed value, or the highscore table.
    PersistentData& reset(Platform& pfrm);
};
