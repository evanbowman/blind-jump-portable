#pragma once


#include "dateTime.hpp"
#include "entity/entity.hpp"
#include "inventory.hpp"
#include "number/numeric.hpp"
#include "powerup.hpp"
#include "settings.hpp"
#include "timeTracker.hpp"
#include "number/endian.hpp"


using Level = s32;
using Score = u32;


class Platform;


// NOTE: Nothing but plain bytes should be stored in the PersistentData
// structure. HostInteger stores its contents as a u8 array, which eliminates
// structure packing headaches. The Inventory class, Settings class, powerup
// class, etc., all store either HostIntegers, or enums derived from u8.


struct PersistentData {
    using Magic = u32;

    static constexpr u32 schema_version = 5;
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
