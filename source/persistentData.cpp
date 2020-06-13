#include "persistentData.hpp"
#include "conf.hpp"
#include "platform/platform.hpp"


PersistentData& PersistentData::reset(Platform& pfrm)
{
    inventory_.remove_non_persistent();
    level_ = 0;
    score_ = 0;
    player_health_ = Conf(pfrm).expect<Conf::Integer>("player", "init_health");

    return *this;
}
