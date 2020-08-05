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

#define HANDLE_MESSAGE(MESSAGE_TYPE)                                           \
    case MESSAGE_TYPE::mt: {                                                   \
        if (message->length_ < sizeof(MESSAGE_TYPE)) {                         \
            return;                                                            \
        }                                                                      \
        MESSAGE_TYPE m;                                                        \
        memcpy(&m, message->data_, sizeof m);                                  \
        listener.receive(m, pfrm, game);                                       \
        pfrm.network_peer().poll_consume(sizeof(MESSAGE_TYPE));                \
        break;                                                                 \
    }

            HANDLE_MESSAGE(EnemyStateSync)
            HANDLE_MESSAGE(PlayerEnteredGate)
            HANDLE_MESSAGE(ItemTaken)
            HANDLE_MESSAGE(PlayerInfo)
            HANDLE_MESSAGE(EnemyHealthChanged)
            HANDLE_MESSAGE(NewLevelIdle)
            HANDLE_MESSAGE(SyncSeed)
            HANDLE_MESSAGE(NewLevelSyncSeed)

        default:
            while (true) {
                // somehow, we've ended up in an erroneous state...
            }
            break;
        }
    }
}

} // namespace net_event
