#pragma once

#include "entity/entity.hpp"
#include "graphics/sprite.hpp"
#include "number/numeric.hpp"
#include "number/random.hpp"
#include "platform/platform.hpp"


// FIXME: Account for endianness!


namespace net_event {

struct Header {
    enum MessageType {
        player_info,
        enemy_health_changed,
        enemy_state_sync,
        sync_seed,
        new_level_sync_seed,
        new_level_idle,
    } message_type_;
};


struct PlayerInfo {
    Header header_;
    Sprite::Size size_;
    u8 texture_index_;
    Vec2<s16> position_;
    Vec2<Float> speed_;
    u8 visible_ : 1;
    u8 weapon_drawn_ : 1;
};


struct EnemyHealthChanged {
    Header header_;
    Entity::Id id_;
    Entity::Health new_health_;
};


struct EnemyStateSync {
    Header header_;
    u8 state_;
    Entity::Id id_;
    Vec2<s16> position_;
};


struct SyncSeed {
    Header header_;
    rng::Generator random_state_;
};


// We make a special distinction for new level seeds, due to a special handshake
// during level transition, which would be complicated by the existing SyncSeed
// message, which are broadcast by the host during other game states.
struct NewLevelSyncSeed {
    Header header_;
    rng::Generator random_state_;
};


struct NewLevelIdle {
    Header header_;
};


template <typename T, Header::MessageType m, typename... Params>
void transmit(Platform& pfrm, Params&&... params)
{
    T message{{m}, std::forward<Params>(params)...};
    pfrm.network_peer().send_message({(byte*)&message, sizeof message});
}


class Listener {
public:
    virtual ~Listener()
    {
    }

    virtual void receive(const PlayerInfo&, Platform&, Game&)
    {
    }
    virtual void receive(const SyncSeed&, Platform&, Game&)
    {
    }
    virtual void receive(const NewLevelSyncSeed&, Platform&, Game&)
    {
    }
    virtual void receive(const NewLevelIdle&, Platform&, Game&)
    {
    }
    virtual void receive(const EnemyHealthChanged&, Platform&, Game&)
    {
    }
    virtual void receive(const EnemyStateSync&, Platform&, Game&)
    {
    }
};


void poll_messages(Platform& pfrm, Game& game, Listener& l);


} // namespace net_event
