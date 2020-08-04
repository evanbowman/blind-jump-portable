#include "network_event.hpp"
#include "platform/platform.hpp"


extern "C" {
void* memcpy(void* destination, const void* source, size_t num);
}


namespace net_event {


void poll_messages(Platform& pfrm, Game& game, Listener& listener)
{
    while (auto message = pfrm.network_peer().poll_message()) {
        if (message->length_ < sizeof(Header)) {
            return;
        }
        Header header;
        memcpy(&header, message->data_, sizeof header);

        switch (header.message_type_) {
        case Header::null:
            pfrm.network_peer().poll_consume(sizeof(Header));
            break;

        case Header::enemy_state_sync: {
            if (message->length_ < sizeof(EnemyStateSync)) {
                return;
            }
            EnemyStateSync s;
            memcpy(&s, message->data_, sizeof s);
            listener.receive(s, pfrm, game);
            pfrm.network_peer().poll_consume(sizeof(EnemyStateSync));
            break;
        }

        case Header::item_taken: {
            if (message->length_ < sizeof(ItemTaken)) {
                return;
            }
            ItemTaken t;
            memcpy(&t, message->data_, sizeof t);
            listener.receive(t, pfrm, game);
            pfrm.network_peer().poll_consume(sizeof(ItemTaken));
            break;
        }

        case Header::player_died: {
            if (message->length_ < sizeof(PlayerDied)) {
                return;
            }
            PlayerDied d;
            memcpy(&d, message->data_, sizeof d);
            listener.receive(d, pfrm, game);
            pfrm.network_peer().poll_consume(sizeof(PlayerDied));
            break;
        }

        case Header::player_info: {
            if (message->length_ < sizeof(PlayerInfo)) {
                return;
            }
            PlayerInfo p;
            memcpy(&p, message->data_, sizeof p);
            listener.receive(p, pfrm, game);
            pfrm.network_peer().poll_consume(sizeof(PlayerInfo));
            break;
        }

        case net_event::Header::enemy_health_changed: {
            if (message->length_ < sizeof(EnemyHealthChanged)) {
                return;
            }
            EnemyHealthChanged ehc;
            memcpy(&ehc, message->data_, sizeof ehc);
            listener.receive(ehc, pfrm, game);
            pfrm.network_peer().poll_consume(sizeof(EnemyHealthChanged));
            break;
        }

        case Header::new_level_idle: {
            if (message->length_ < sizeof(NewLevelIdle)) {
                return;
            }
            NewLevelIdle l;
            memcpy(&l, message->data_, sizeof l);
            listener.receive(l, pfrm, game);
            pfrm.network_peer().poll_consume(sizeof(NewLevelIdle));
            break;
        }

        case Header::sync_seed: {
            if (message->length_ < sizeof(SyncSeed)) {
                return;
            }
            SyncSeed s;
            memcpy(&s, message->data_, sizeof s);
            listener.receive(s, pfrm, game);
            pfrm.network_peer().poll_consume(sizeof(SyncSeed));
            break;
        }

        case Header::new_level_sync_seed: {
            if (message->length_ < sizeof(NewLevelSyncSeed)) {
                return;
            }
            NewLevelSyncSeed s;
            memcpy(&s, message->data_, sizeof s);
            listener.receive(s, pfrm, game);
            pfrm.network_peer().poll_consume(sizeof(NewLevelSyncSeed));
            break;
        }

        default:
            while (true) {
                // somehow, we've ended up in an erroneous state...
            }
            break;
        }
    }
}

} // namespace net_event
