#include <stdbool.h>
#include "rumble.h"


typedef unsigned int u32;
typedef unsigned short u16;


#define GPIO_PORT_DATA        (*(volatile u16 *)0x80000C4)
#define GPIO_PORT_DIRECTION   (*(volatile u16 *)0x80000C6)

#define REG_BASE 0x04000000

#define REG_RCNT              *(volatile u16*)(REG_BASE + 0x134)
#define REG_SIOCNT            *(volatile u16*)(REG_BASE + 0x128)
#define REG_SIODATA32         *(volatile u32*)(REG_BASE + 0x120)

#define SIO_START (1<<7)


static char const gbp_comms_stage0_data[] = {"NINTENDO"};


static bool gbp_configured;
static enum RumbleState rumble_state;


enum GBPCommsStage {
    gbp_comms_nintendo_handshake,
    gbp_comms_check_magic1,
    gbp_comms_check_magic2,
    gbp_comms_rumble,
    gbp_comms_finalize
};


static struct {
    enum GBPCommsStage stage_;
    u32 serial_in_;
    u16 index_;
    u16 out_0_;
    u16 out_1_;
} gbp_comms;


static void gbp_serial_isr()
{
    u32 out = 0;
    u16 in_0;
    u16 in_1;

    gbp_comms.serial_in_ = REG_SIODATA32;

    switch (gbp_comms.stage_) {
    case gbp_comms_nintendo_handshake:
        in_0 = gbp_comms.serial_in_ >> 16;
        in_1 = gbp_comms.serial_in_;

        if (in_1 == 0x8002) {
            out = 0x10000010;
            gbp_comms.stage_ = gbp_comms_check_magic1;
            break;
        }

        if (in_0 != gbp_comms.out_1_) {
            gbp_comms.index_ = 0;
        }

        if (gbp_comms.index_ <= 3) {
            if (gbp_comms.serial_in_ ==
                (u32) ~(gbp_comms.out_1_ | (gbp_comms.out_0_ << 16))) {
                gbp_comms.index_ += 1;
            }
            gbp_comms.out_0_ = ((const u16*)gbp_comms_stage0_data)[gbp_comms.index_];
        } else {
            gbp_comms.out_0_ = 0x8000;
        }

        gbp_comms.out_1_ = ~in_1;
        out = gbp_comms.out_1_ | (gbp_comms.out_0_ << 16);
        break;

    case gbp_comms_check_magic1:
        if (gbp_comms.serial_in_ == 0x10000010) {
            out = 0x20000013;
            gbp_comms.stage_ = gbp_comms_check_magic2;
        } else {
            gbp_comms.stage_ = gbp_comms_finalize;
        }
        break;

    case gbp_comms_check_magic2:
        if (gbp_comms.serial_in_ == 0x20000013) {
            out = 0x40000004;
            gbp_comms.stage_ = gbp_comms_rumble;
        } else {
            gbp_comms.stage_ = gbp_comms_finalize;
        }
        break;

    case gbp_comms_rumble:
        if (gbp_comms.serial_in_ == 0x30000003) {
            out = rumble_state;
        } else {
            gbp_comms.stage_ = gbp_comms_finalize;
        }
        break;

    case gbp_comms_finalize:
        gbp_comms.serial_in_ = 0;
        gbp_comms.stage_ = gbp_comms_nintendo_handshake;
        gbp_comms.index_ = 0;
        gbp_comms.out_0_ = 0;
        gbp_comms.out_1_ = 0;
        return;
    }

    REG_SIODATA32 = out;
    REG_SIOCNT |= SIO_START;
}


static void gbp_serial_start()
{
    if (gbp_comms.serial_in_ != 0x30000003) {
        REG_RCNT = 0x0;
        REG_SIOCNT = 0x1008;
        REG_SIOCNT |= 0x4000;
        REG_SIOCNT &= ~1;
        REG_SIOCNT |= 0x0080;
    }
}


void rumble_init(struct RumbleGBPConfig* config)
{
    rumble_state = rumble_stop;

    if (config) {
        config->serial_irq_setup_(gbp_serial_isr);
        gbp_configured = true;
        gbp_comms.serial_in_ = 0;
        gbp_comms.stage_ = gbp_comms_nintendo_handshake;
        gbp_comms.index_ = 0;
        gbp_comms.out_0_ = 0;
        gbp_comms.out_1_ = 0;
    } else {
        gbp_configured = false;

    }
}


void rumble_update()
{
    if (gbp_configured) {
        gbp_serial_start();
    }
}


void rumble_set_state(enum RumbleState state)
{
    rumble_state = state;

    if (!gbp_configured) {
        GPIO_PORT_DIRECTION = 1 << 3;
        GPIO_PORT_DATA = (rumble_state == rumble_start) << 3;
    }
}
