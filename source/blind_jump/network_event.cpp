#include "network_event.hpp"
#include "platform/platform.hpp"


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
            continue;

#define HANDLE_MESSAGE(MESSAGE_TYPE)                                           \
    case MESSAGE_TYPE::mt: {                                                   \
        NET_EVENT_SIZE_CHECK(MESSAGE_TYPE)                                     \
        if (message->length_ < sizeof(MESSAGE_TYPE)) {                         \
            return;                                                            \
        }                                                                      \
        MESSAGE_TYPE m;                                                        \
        memcpy(&m, message->data_, sizeof m);                                  \
        pfrm.network_peer().poll_consume(sizeof(MESSAGE_TYPE));                \
        listener.receive(m, pfrm, game);                                       \
        continue;                                                              \
    }

            HANDLE_MESSAGE(EnemyStateSync)
            HANDLE_MESSAGE(PlayerEnteredGate)
            HANDLE_MESSAGE(PlayerHealthChanged)
            HANDLE_MESSAGE(ItemTaken)
            HANDLE_MESSAGE(PlayerInfo)
            HANDLE_MESSAGE(EnemyHealthChanged)
            HANDLE_MESSAGE(NewLevelIdle)
            HANDLE_MESSAGE(SyncSeed)
            HANDLE_MESSAGE(NewLevelSyncSeed)
            HANDLE_MESSAGE(ItemChestOpened)
            HANDLE_MESSAGE(ItemChestShared)
            HANDLE_MESSAGE(QuickChat)
            HANDLE_MESSAGE(PlayerSpawnLaser)
            HANDLE_MESSAGE(PlayerDied)
            HANDLE_MESSAGE(DataStreamAvail)
            HANDLE_MESSAGE(DataStreamReadResponse)
            HANDLE_MESSAGE(DataStreamReadRequest)
            HANDLE_MESSAGE(DataStreamDoneReading)
            HANDLE_MESSAGE(ProgramVersion)
            HANDLE_MESSAGE(LethargyActivated)
            HANDLE_MESSAGE(Disconnect)
            HANDLE_MESSAGE(HealthTransfer)
            HANDLE_MESSAGE(BossSwapTarget)
        }

        error(pfrm, "garbled message!?");
        pfrm.network_peer().disconnect();
        return;
    }
}

} // namespace net_event
