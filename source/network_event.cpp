#include "network_event.hpp"
#include "platform/platform.hpp"


extern "C" {
void* memcpy(void* destination, const void* source, size_t num);
}


namespace net_event {


void poll_messages(Platform& pfrm, Game& game, Listener& listener)
{
    u32 read_position = 0;
    while (auto message = pfrm.network_peer().poll_messages(read_position)) {
        Header header;
        memcpy(&header, message->data_, sizeof header);

        switch (header.message_type_) {
        case Header::enemy_state_sync: {
            read_position += sizeof(EnemyStateSync);
            EnemyStateSync s;
            memcpy(&s, message->data_, sizeof s);
            listener.receive(s, pfrm, game);
            break;
        }

        case Header::item_taken: {
            read_position += sizeof(ItemTaken);
            ItemTaken t;
            memcpy(&t, message->data_, sizeof t);
            listener.receive(t, pfrm, game);
            break;
        }

        case Header::player_died: {
            read_position += sizeof(PlayerDied);
            PlayerDied d;
            memcpy(&d, message->data_, sizeof d);
            listener.receive(d, pfrm, game);
            break;
        }

        case Header::player_info: {
            read_position += sizeof(PlayerInfo);
            PlayerInfo p;
            memcpy(&p, message->data_, sizeof p);
            listener.receive(p, pfrm, game);
            break;
        }

        case net_event::Header::enemy_health_changed: {
            read_position += sizeof(EnemyHealthChanged);
            EnemyHealthChanged ehc;
            memcpy(&ehc, message->data_, sizeof ehc);
            listener.receive(ehc, pfrm, game);
            break;
        }

        case Header::new_level_idle: {
            read_position += sizeof(NewLevelIdle);
            NewLevelIdle l;
            memcpy(&l, message->data_, sizeof l);
            listener.receive(l, pfrm, game);
            break;
        }

        case Header::sync_seed: {
            read_position += sizeof(SyncSeed);
            SyncSeed s;
            memcpy(&s, message->data_, sizeof s);
            listener.receive(s, pfrm, game);
            break;
        }

        case Header::new_level_sync_seed: {
            read_position += sizeof(SyncSeed);
            NewLevelSyncSeed s;
            memcpy(&s, message->data_, sizeof s);
            listener.receive(s, pfrm, game);
            break;
        }
        }
    }
}

} // namespace net_event
