////////////////////////////////////////////////////////////////////////////////
//
// All of the code in this file will be compiled as arm code, and placed in the
// IWRAM secion of the executable. The system has limited memory for IWRAM
// calls, so limit this file to performace critical code, or code that must be
// defined in IWRAM.
//
////////////////////////////////////////////////////////////////////////////////


#ifdef __GBA__
#define IWRAM_CODE __attribute__((section(".iwram"), long_call))
#else
#define IWRAM_CODE
#endif // __GBA__


// Because the cartridge interrupt handler runs when the cartridge is removed,
// it obviously cannot be defined in gamepak rom! So we have to put the code in
// IWRAM.
IWRAM_CODE
void cartridge_interrupt_handler()
{
    #define REG_DISPCNT *(unsigned*)0x4000000

    REG_DISPCNT = 0;

    while (true) ;
}
