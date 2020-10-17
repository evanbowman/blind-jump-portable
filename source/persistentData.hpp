#pragma once


#include "dateTime.hpp"
#include "entity/entity.hpp"
#include "inventory.hpp"
#include "number/numeric.hpp"
#include "powerup.hpp"
#include "settings.hpp"
#include "timeTracker.hpp"


using Level = s32;
using Score = u64;


class Platform;


struct PersistentData {
    static constexpr u32 schema_version = 4;
    static constexpr u32 magic_val = 0xCA55E77E + schema_version;

    using Magic = u32;

    Magic magic_ = magic_val;
    u32 seed_ = 11001;
    Level level_ = 0;
    Score score_ = 0;
    Entity::Health player_health_ = 3;

    using HighScores = std::array<Score, 8>;
    HighScores highscores_ = {0};

    Inventory inventory_;

    std::array<Powerup, Powerup::max_> powerups_;
    u32 powerup_count_ = 0;

    Settings settings_;

    DateTime timestamp_ = {{0, 0, 0}, 0, 0, 0};

    TimeTracker speedrun_clock_ = {0};

    bool displayed_health_warning_ = false;

    // This save block has never been written.
    bool clean_ = true;

    void store_powerups(const Powerups& powerups);

    // Reset level back to zero, initialize various things. Does not overwrite
    // the seed value, or the highscore table.
    PersistentData& reset(Platform& pfrm);
};
