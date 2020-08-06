#pragma once

#include "entity/entity.hpp"
#include "graphics/sprite.hpp"
#include "number/numeric.hpp"
#include "number/random.hpp"
#include "platform/platform.hpp"


// FIXME: Account for endianness! All of my devices are little endian.


namespace net_event {

struct Header {
    enum MessageType : u8 {
        null, // Important! The platform does not guarantee transmission of
              // messages full of zero bytes, so the 0th enumeration must not be
              // used (may not be received).
        player_info,
        player_entered_gate,
        enemy_health_changed,
        enemy_state_sync,
        sync_seed,
        new_level_sync_seed,
        new_level_idle,
        item_taken,
        item_chest_opened,
        quick_chat,
    } message_type_;
};
static_assert(sizeof(Header) == 1);


#define NET_EVENT_SIZE_CHECK(TYPE)                                             \
    static_assert(sizeof(TYPE) <= Platform::NetworkPeer::max_message_size);


struct PlayerInfo {
    Header header_;
    // TODO: replace bitfields with single var, provide bitmasks
    Sprite::Size size_ : 1;

    enum DisplayColor { none, injured, got_coin, got_heart };

    DisplayColor disp_color_ : 3;
    u8 color_amount_ : 4; // Right-shifted by four


    u8 unused2_[3];
    u8 visible_ : 1;
    u8 weapon_drawn_ : 1;
    u8 texture_index_ : 6;

    // For speed values, the player's speed ranges from float -1.5 to
    // 1.5. Therefore, we can save a lot of space in the message by using single
    // signed bytes, and representing the values as fixed point.
    s8 x_speed_; // Fixed point with implicit 10^1 decimal
    s8 y_speed_; // Fixed point with implicit 10^1 decimal

    s16 x_;
    s16 y_;

    static const auto mt = Header::MessageType::player_info;
};
NET_EVENT_SIZE_CHECK(PlayerInfo)


struct PlayerEnteredGate {
    Header header_;
    u8 unused_[11];

    static const auto mt = Header::MessageType::player_entered_gate;
};
NET_EVENT_SIZE_CHECK(PlayerEnteredGate)


struct ItemChestOpened {
    Header header_;
    u8 unused_[7];
    Entity::Id id_;

    static const auto mt = Header::MessageType::item_chest_opened;
};
NET_EVENT_SIZE_CHECK(ItemChestOpened)


// Currently unused. In order to use ItemTaken, we'll need to keep entity ids
// properly synchronized. If even one extra enemy bullet is spawned by one of
// the peers, the ids will be out of sequence, rendering the ItemTaken message
// ineffective, or worse.
struct ItemTaken {
    Header header_;
    u8 unused_[7];
    Entity::Id id_;

    static const auto mt = Header::MessageType::item_taken;
};
NET_EVENT_SIZE_CHECK(ItemTaken)


struct EnemyHealthChanged {
    Header header_;
    u8 unused_[3];
    Entity::Id id_;
    Entity::Health new_health_;

    static const auto mt = Header::MessageType::enemy_health_changed;
};
NET_EVENT_SIZE_CHECK(EnemyHealthChanged)


struct EnemyStateSync {
    Header header_;
    u8 unused_[2];
    u8 state_;
    s16 x_;
    s16 y_;
    Entity::Id id_;

    static const auto mt = Header::MessageType::enemy_state_sync;
};
NET_EVENT_SIZE_CHECK(EnemyStateSync)


struct SyncSeed {
    Header header_;
    u8 unused_[7];
    rng::Generator random_state_;

    static const auto mt = Header::MessageType::sync_seed;
};
NET_EVENT_SIZE_CHECK(SyncSeed)


// We make a special distinction for new level seeds, due to a special handshake
// during level transition, which would be complicated by the existing SyncSeed
// message, which are broadcast by the host during other game states.
struct NewLevelSyncSeed {
    Header header_;
    u8 unused_[7];
    rng::Generator random_state_;

    static const auto mt = Header::MessageType::new_level_sync_seed;
};
NET_EVENT_SIZE_CHECK(NewLevelSyncSeed)


struct NewLevelIdle {
    Header header_;
    u8 unused_[11];

    static const auto mt = Header::MessageType::new_level_idle;
};
NET_EVENT_SIZE_CHECK(NewLevelIdle)


struct QuickChat {
    Header header_;
    char message_[11];

    static const auto mt = Header::MessageType::quick_chat;
};
NET_EVENT_SIZE_CHECK(QuickChat)


template <typename T> void transmit(Platform& pfrm, T& message)
{
    static_assert(sizeof(T) <= Platform::NetworkPeer::max_message_size);

    message.header_.message_type_ = T::mt;

    // Most of the time, should only be one iteration...
    while (
        pfrm.network_peer().is_connected() and
        not pfrm.network_peer().send_message({(byte*)&message, sizeof message}))
        ;
}


class Listener {
public:
    virtual ~Listener()
    {
    }

    virtual void receive(const PlayerInfo&, Platform&, Game&)
    {
    }
    virtual void receive(const PlayerEnteredGate&, Platform&, Game&)
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
    virtual void receive(const ItemTaken&, Platform&, Game&)
    {
    }
    virtual void receive(const ItemChestOpened&, Platform&, Game&)
    {
    }
    virtual void receive(const QuickChat&, Platform&, Game&)
    {
    }
};


void poll_messages(Platform& pfrm, Game& game, Listener& l);


// don't care, but don't want the message queue to overflow either
inline void ignore_all(Platform& pfrm, Game& game)
{
    Listener l;
    poll_messages(pfrm, game, l);
}


} // namespace net_event
