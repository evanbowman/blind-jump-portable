#include "persistentData.hpp"
#include "platform/platform.hpp"


PersistentData& PersistentData::reset(Platform& pfrm)
{
    inventory_.remove_non_persistent();
    level_.set(0);
    score_.set(0);
    player_health_.set(3);
    powerup_count_.set(0);
    speedrun_clock_.reset(0);

    return *this;
}


void PersistentData::store_powerups(const Powerups& powerups)
{
    std::copy(powerups.begin(), powerups.end(), powerups_.begin());
    powerup_count_.set(powerups.size());
}
