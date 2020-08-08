#pragma once

#include "entity/entity.hpp"
#include "graphics/sprite.hpp"
#include "number/endian.hpp"
#include "number/numeric.hpp"
#include "number/random.hpp"
#include "platform/platform.hpp"


namespace net_event {

struct Header {
    enum MessageType : u8 {
        null, // Important! The platform does not guarantee transmission of
              // messages full of zero bytes, so the 0th enumeration must not be
              // used (may not be received).
        player_info,
        player_spawn_laser,
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
    static_assert(sizeof(TYPE) == Platform::NetworkPeer::max_message_size);


struct PlayerInfo {
    Header header_;

    enum DisplayColor { none, injured, got_coin, got_heart };

    static const u8 opt1_size_mask = 0b10000000;
    static const u8 opt1_color_mask = 0b01110000;
    static const u8 opt1_color_amount_mask = 0b00001111;

    static const u8 opt1_size_shift = 7;
    static const u8 opt1_color_shift = 4;

    u8 opt1_;

    static const u8 opt2_visible_mask = 0b10000000;
    static const u8 opt2_weapon_drawn_mask = 0b01000000;
    static const u8 opt2_texture_mask = 0b00111111;

    static const u8 opt2_visible_shift = 7;
    static const u8 opt2_weapon_drawn_shift = 6;

    u8 opt2_;


    void set_sprite_size(Sprite::Size size)
    {
        opt1_ &= ~opt1_size_mask;
        opt1_ |= opt1_size_mask & (size << opt1_size_shift);
    }

    Sprite::Size get_sprite_size() const
    {
        return static_cast<Sprite::Size>((opt1_ & opt1_size_mask) >>
                                         opt1_size_shift);
    }

    void set_display_color(DisplayColor dc)
    {
        opt1_ &= ~opt1_color_mask;
        opt1_ |= opt1_color_mask & (dc << opt1_color_shift);
    }

    DisplayColor get_display_color() const
    {
        return static_cast<DisplayColor>((opt1_ & opt1_color_mask) >>
                                         opt1_color_shift);
    }

    void set_color_amount(u8 color_amount)
    {
        opt1_ &= ~opt1_color_amount_mask;
        opt1_ |= color_amount >> 4;
    }

    u8 get_color_amount() const
    {
        return opt1_ << 4;
    }

    void set_visible(bool visible)
    {
        opt2_ &= ~opt2_visible_mask;
        opt2_ |= opt2_visible_mask & (visible << opt2_visible_shift);
    }

    bool get_visible() const
    {
        return (opt2_ & opt2_visible_mask) >> opt2_visible_shift;
    }

    void set_weapon_drawn(bool weapon_drawn)
    {
        opt2_ &= ~opt2_weapon_drawn_mask;
        opt2_ |=
            opt2_weapon_drawn_mask & (weapon_drawn << opt2_weapon_drawn_shift);
    }

    bool get_weapon_drawn() const
    {
        return (opt2_ & opt2_weapon_drawn_mask) >> opt2_weapon_drawn_shift;
    }

    void set_texture_index(u8 val)
    {
        opt2_ &= ~opt2_texture_mask;
        opt2_ |= opt2_texture_mask & val;
    }

    u8 get_texture_index() const
    {
        return opt2_ & opt2_texture_mask;
    }

    u8 unused2_[3];

    // For speed values, the player's speed ranges from float -1.5 to
    // 1.5. Therefore, we can save a lot of space in the message by using single
    // signed bytes, and representing the values as fixed point.
    s8 x_speed_; // Fixed point with implicit 10^1 decimal
    s8 y_speed_; // Fixed point with implicit 10^1 decimal

    host_s16 x_;
    host_s16 y_;

    static const auto mt = Header::MessageType::player_info;
};
NET_EVENT_SIZE_CHECK(PlayerInfo)


struct PlayerSpawnLaser {
    Header header_;
    Cardinal dir_;
    host_s16 x_;
    host_s16 y_;

    u8 more_unused_[6];

    static const auto mt = Header::MessageType::player_spawn_laser;
};
NET_EVENT_SIZE_CHECK(PlayerSpawnLaser)


struct PlayerEnteredGate {
    Header header_;

    u8 unused_[11];

    static const auto mt = Header::MessageType::player_entered_gate;
};
NET_EVENT_SIZE_CHECK(PlayerEnteredGate)


struct ItemChestOpened {
    Header header_;
    HostInteger<Entity::Id> id_;

    u8 unused_[7];

    static const auto mt = Header::MessageType::item_chest_opened;
};
NET_EVENT_SIZE_CHECK(ItemChestOpened)


// Currently unused. In order to use ItemTaken, we'll need to keep entity ids
// properly synchronized. If even one extra enemy bullet is spawned by one of
// the peers, the ids will be out of sequence, rendering the ItemTaken message
// ineffective, or worse.
struct ItemTaken {
    Header header_;
    HostInteger<Entity::Id> id_;

    u8 unused_[7];

    static const auto mt = Header::MessageType::item_taken;
};
NET_EVENT_SIZE_CHECK(ItemTaken)


struct EnemyHealthChanged {
    Header header_;
    HostInteger<Entity::Id> id_;
    HostInteger<Entity::Health> new_health_;

    u8 unused_[3];

    static const auto mt = Header::MessageType::enemy_health_changed;
};
NET_EVENT_SIZE_CHECK(EnemyHealthChanged)


struct EnemyStateSync {
    Header header_;
    u8 state_;
    host_s16 x_;
    host_s16 y_;
    HostInteger<Entity::Id> id_;

    u8 unused_[2];

    static const auto mt = Header::MessageType::enemy_state_sync;
};
NET_EVENT_SIZE_CHECK(EnemyStateSync)


struct SyncSeed {
    Header header_;
    HostInteger<rng::Generator> random_state_;

    u8 unused_[7];

    static const auto mt = Header::MessageType::sync_seed;
};
NET_EVENT_SIZE_CHECK(SyncSeed)


// We make a special distinction for new level seeds, due to a special handshake
// during level transition, which would be complicated by the existing SyncSeed
// message, which are broadcast by the host during other game states.
struct NewLevelSyncSeed {
    Header header_;
    HostInteger<rng::Generator> random_state_;

    u8 unused_[7];

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
    host_u32 message_;

    char unused_[7];

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
    virtual void receive(const PlayerSpawnLaser&, Platform&, Game&)
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
