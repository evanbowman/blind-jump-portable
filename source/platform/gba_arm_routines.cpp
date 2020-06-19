////////////////////////////////////////////////////////////////////////////////
//
// All of the code in this file will be compiled as arm code, and placed in the
// IWRAM section of the executable. The system has limited memory for IWRAM
// calls, so limit this file to performace critical code, or code that must be
// defined in IWRAM.
//
////////////////////////////////////////////////////////////////////////////////


// #ifdef __GBA__
// #define IWRAM_CODE __attribute__((section(".iwram"), long_call))
// #else
// #define IWRAM_CODE
// #endif // __GBA__


#include "gba_color.hpp"
#include "number/numeric.hpp"
#include "/opt/devkitpro/libgba/include/gba_interrupt.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#include "/opt/devkitpro/libgba/include/gba_systemcalls.h"
#pragma GCC diagnostic pop


// Because the cartridge interrupt handler runs when the cartridge is removed,
// it obviously cannot be defined in gamepak rom! So we have to put the code in
// IWRAM.
IWRAM_CODE
void cartridge_interrupt_handler()
{
    SoftReset(RAM_RESTART);

    while (true)
        ;
}
