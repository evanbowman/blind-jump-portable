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

    return *this;
}


void PersistentData::store_powerups(const Powerups& powerups)
{
    std::copy(powerups.begin(), powerups.end(), powerups_.begin());
    powerup_count_ = powerups.size();
}
