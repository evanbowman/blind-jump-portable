#include "persistentData.hpp"
#include "conf.hpp"
#include "platform/platform.hpp"


PersistentData& PersistentData::reset(Platform& pfrm)
{
    inventory_.remove_non_persistent();
    level_ = 0;
    score_ = 0;
    player_health_ = Conf(pfrm).expect<Conf::Integer>("player", "init_health");
    powerup_count_ = 0;

    return *this;
}


void PersistentData::store_powerups(const Powerups& powerups)
{
    std::copy(powerups.begin(), powerups.end(), powerups_.begin());
    powerup_count_ = powerups.size();
}
