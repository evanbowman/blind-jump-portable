
////////////////////////////////////////////////////////////////////////////////
//
//
// PSP Platform
//
//
////////////////////////////////////////////////////////////////////////////////

#include "platform/platform.hpp"
#include <pspkernel.h>
#include <pspdebug.h>


PSP_MODULE_INFO("Blind Jump", 0, 1, 0);


int main(int argc, char** argv)
{
    Platform pf;
}


int exit_callback(int arg1, int arg2, void* common){
    sceKernelExitGame();
    return 0;
}

int callback_thread(SceSize args, void* argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();

    while (true) {
        // ...
    }

    return 0;
}


int setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
    return thid;
}


Platform::Platform()
{
    setup_callbacks();

    pspDebugScreenInit();
    pspDebugScreenPrintf("Blind Jump Init");

    while (true) ;

    return 0;
}
