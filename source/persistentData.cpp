#include "persistentData.hpp"
#include "platform/platform.hpp"


PersistentData& PersistentData::reset(Platform& pfrm)
{
    inventory_.remove_non_persistent();
    level_ = 0;
    score_ = 0;
    player_health_ = 3;
    powerup_count_ = 0;
    speedrun_clock_.reset(0);
    oxygen_remaining_.reset([&] {
        switch (settings_.difficulty_) {
        case Settings::Difficulty::easy:
            return 60 * 45;

        case Settings::Difficulty::normal:
        case Settings::Difficulty::count:
            break;

        case Settings::Difficulty::hard:
        case Settings::Difficulty::survival:
            return 60 * 25;
        }

        return 60 * 30;
    }());

    return *this;
}


void PersistentData::store_powerups(const Powerups& powerups)
{
    std::copy(powerups.begin(), powerups.end(), powerups_.begin());
    powerup_count_ = powerups.size();
}
