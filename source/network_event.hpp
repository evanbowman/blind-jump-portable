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
        null,
        player_info,
        player_died,
        enemy_health_changed,
        enemy_state_sync,
        sync_seed,
        new_level_sync_seed,
        new_level_idle,
        item_taken,
    } message_type_;
};


struct PlayerInfo {
    Header header_;
    Sprite::Size size_;
    u8 texture_index_;
    s16 x_;
    s16 y_;
    Float x_speed_;
    Float y_speed_;
    u8 visible_ : 1;
    u8 weapon_drawn_ : 1;

    static const auto mt = Header::MessageType::player_info;
};


struct PlayerDied {
    Header header_;

    static const auto mt = Header::MessageType::player_died;
};


// Currently unused. In order to use ItemTaken, we'll need to keep entity ids
// properly synchronized. If even one extra enemy bullet is spawned by one of
// the peers, the ids will be out of sequence, rendering the ItemTaken message
// ineffective, or worse.
struct ItemTaken {
    Header header_;
    Entity::Id id_;

    static const auto mt = Header::MessageType::item_taken;
};


struct EnemyHealthChanged {
    Header header_;
    Entity::Id id_;
    Entity::Health new_health_;

    static const auto mt = Header::MessageType::enemy_health_changed;
};


struct EnemyStateSync {
    Header header_;
    u8 state_;
    Entity::Id id_;
    s16 x_;
    s16 y_;

    static const auto mt = Header::MessageType::enemy_state_sync;
};


struct SyncSeed {
    Header header_;
    rng::Generator random_state_;

    static const auto mt = Header::MessageType::sync_seed;
};


// We make a special distinction for new level seeds, due to a special handshake
// during level transition, which would be complicated by the existing SyncSeed
// message, which are broadcast by the host during other game states.
struct NewLevelSyncSeed {
    Header header_;
    rng::Generator random_state_;

    static const auto mt = Header::MessageType::new_level_sync_seed;
};


struct NewLevelIdle {
    Header header_;

    static const auto mt = Header::MessageType::new_level_idle;
};


template <typename T, typename... Params>
void transmit(Platform& pfrm, Params&&... params)
{
    T message{{T::mt}, std::forward<Params>(params)...};
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
    virtual void receive(const PlayerDied&, Platform&, Game&)
    {
    }
    virtual void receive(const ItemTaken&, Platform&, Game&)
    {
    }
};


void poll_messages(Platform& pfrm, Game& game, Listener& l);


} // namespace net_event
