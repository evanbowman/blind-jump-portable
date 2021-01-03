#pragma once


#ifdef __cplusplus
extern "C" {
#endif


struct RumbleGBPConfig {
    void (*serial_irq_setup_)(void (*rumble_isr)(void));
};


enum RumbleState {
    rumble_start     = 0x40000026,
    rumble_stop      = 0x40000004,
    rumble_hard_stop = 0x40000015,
};


void rumble_set_state(enum RumbleState state);


void rumble_init(struct RumbleGBPConfig* optional_gbp_config);


/* should be called once per frame */
void rumble_update();


#ifdef __cplusplus
}
#endif
