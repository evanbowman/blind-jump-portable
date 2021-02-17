
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

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    pspDebugScreenInit();
    pspDebugScreenPrintf("Blind Jump Init");

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////


std::optional<Bitvector<int(Key::count)>> missed_keys;


void Platform::Keyboard::register_controller(const ControllerInfo& info)
{
    // ...
}


void Platform::Keyboard::poll()
{
    std::copy(std::begin(states_), std::end(states_), std::begin(prev_));

    SceCtrlData buttonInput;

    sceCtrlPeekBufferPositive(&buttonInput, 1);

    if (buttonInput.Buttons) {
        states_[int(Key::start)] = buttonInput.Buttons & PSP_CTRL_START;
        states_[int(Key::select)] = buttonInput.Buttons & PSP_CTRL_SELECT;
        states_[int(Key::right)] = buttonInput.Buttons & PSP_CTRL_RIGHT;
        states_[int(Key::left)] = buttonInput.Buttons & PSP_CTRL_LEFT;
        states_[int(Key::down)] =  buttonInput.Buttons & PSP_CTRL_DOWN;
        states_[int(Key::up)] = buttonInput.Buttons & PSP_CTRL_UP;
        states_[int(Key::alt_1)] = buttonInput.Buttons & PSP_CTRL_RTRIGGER;
        states_[int(Key::alt_2)] = buttonInput.Buttons & PSP_CTRL_LTRIGGER;
        states_[int(Key::action_1)] = buttonInput.Buttons & PSP_CTRL_CROSS;
        states_[int(Key::action_2)] = buttonInput.Buttons & PSP_CTRL_CIRCLE;
    }

    if (UNLIKELY(static_cast<bool>(::missed_keys))) {
        for (int i = 0; i < (int)Key::count; ++i) {
            if ((*::missed_keys)[i]) {
                states_[i] = true;
            }
        }
        ::missed_keys.reset();
    }
}
