////////////////////////////////////////////////////////////////////////////////
//
// All of the code in this file will be compiled as arm code, and placed in the
// IWRAM section of the executable. The system has limited memory for IWRAM
// calls, so limit this file to performace critical code, or code that must be
// defined in IWRAM.
//
////////////////////////////////////////////////////////////////////////////////


#ifdef __GBA__
#define IWRAM_CODE __attribute__((section(".iwram"), long_call))
#else
#define IWRAM_CODE
#endif // __GBA__


#include "gba.h"

// Because the cartridge interrupt handler runs when the cartridge is removed,
// it obviously cannot be defined in gamepak rom! So we have to put the code in
// IWRAM.
IWRAM_CODE
void cartridge_interrupt_handler()
{
    Stop();
}


#include "gba_platform_soundcontext.hpp"



extern SoundContext snd_ctx;



#define REG_SGFIFOA *(volatile u32*)0x40000A0


// NOTE: The primary audio mixing routine.
IWRAM_CODE
void audio_update_fast_isr()
{
    alignas(4) AudioSample mixing_buffer[8];

    auto& music_pos = snd_ctx.music_track_pos;
    const auto music_len = snd_ctx.music_track_length;


    if (music_pos > music_len) {
        music_pos = 0;
    }
    // Load 8 music samples upfront (in chunks of four), to try to take
    // advantage of sequential cartridge reads.
    auto music_in = (u32*)mixing_buffer;
    *(music_in++) = ((u32*)(snd_ctx.music_track))[music_pos++];
    *(music_in) = ((u32*)(snd_ctx.music_track))[music_pos++];

    auto it = snd_ctx.active_sounds.begin();
    while (it not_eq snd_ctx.active_sounds.end()) {

        // Cache the position index into the sound data, then pre-increment by
        // eight. Incrementing by eight upfront is better than checking if index
        // + 8 is greater than sound length and then performing index += 8 after
        // doing the mixing, saves an addition.
        int pos = it->position_;
        it->position_ += 8;

        // Aha! __builtin_expect actually results in measurably better latency
        // for once!
        if (UNLIKELY(it->position_ >= it->length_)) {
            it = snd_ctx.active_sounds.erase(it);
        } else {
            // Manually unrolled loop below. Better performance during testing,
            // uses more iwram of course.
            //
            // Note: storing the mixing buffer in a pointer and incrementing the
            // write location resulted in notably better performance than
            // subscript indexing into mixing_buffer with literal indices
            // (mixing_buffer[0], mixing_buffer[1], etc.).
            AudioSample* out = mixing_buffer;
            *(out++) += it->data_[pos++]; // 0
            *(out++) += it->data_[pos++]; // 1
            *(out++) += it->data_[pos++]; // 2
            *(out++) += it->data_[pos++]; // 3
            *(out++) += it->data_[pos++]; // 4
            *(out++) += it->data_[pos++]; // 5
            *(out++) += it->data_[pos++]; // 6
            *(out) += it->data_[pos++];   // 7
            ++it;
        }
    }

    auto sound_out = (u32*)mixing_buffer;

    // NOTE: yeah the register is a FIFO
    REG_SGFIFOA = *(sound_out++);
    REG_SGFIFOA = *(sound_out);
}
