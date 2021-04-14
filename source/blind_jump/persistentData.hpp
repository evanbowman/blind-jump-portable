#pragma once


#include "dateTime.hpp"
#include "entity/entity.hpp"
#include "inventory.hpp"
#include "number/endian.hpp"
#include "number/numeric.hpp"
#include "powerup.hpp"
#include "settings.hpp"
#include "timeTracker.hpp"


using Level = s32;
using Score = u32;


class Platform;


// NOTE: Nothing but plain bytes should be stored in the PersistentData
// structure. HostInteger stores its contents as a u8 array, which eliminates
// structure packing headaches. The Inventory class, Settings class, powerup
// class, etc., all store either HostIntegers, or enums derived from u8.
//
// Additionally, you should be using the fixed width datatypes for everything,
// i.e. s32 instead of a plain int.
//
// The game more-or-less memcpy's the contents of the PersistentData structure
// to the save data target device (whether that be a file on disk, or GBA sram),
// so we need to be careful about anything that might vary across different
// compilers, processors, etc.
//
// Remember that we're using a binary format here! If you change the structure
// of the PersistentData struct, the game may try to incorrectly read existing
// save data files belonging to users who upgraded from an old version of the
// game. So, when you make any changes whatsoever to the layout of the
// PersistentData class, remember to increment the schema_version constant.


struct PersistentData {
    using Magic = u32;

    static constexpr u32 schema_version = 9;
    static constexpr Magic magic_val = 0xCA55E77E + schema_version;

    PersistentData()
    {
        magic_.set(magic_val);
        seed_.set(11001);
        level_.set(0);
        score_.set(0);
        player_health_.set(3);

        for (auto& hs : highscores_) {
            hs.set(0);
        }

        powerup_count_.set(0);
    }

    HostInteger<Magic> magic_;
    HostInteger<u32> seed_;
    HostInteger<Level> level_;
    HostInteger<Score> score_;
    HostInteger<Entity::Health> player_health_;

    using HighScores = HostInteger<Score>[8];
    HighScores highscores_;

    Inventory inventory_;

    std::array<Powerup, Powerup::max_> powerups_;
    HostInteger<u32> powerup_count_;

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
