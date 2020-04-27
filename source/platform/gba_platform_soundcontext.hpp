#pragma once


#include "memory/buffer.hpp"


using AudioSample = s8;


struct ActiveSoundInfo {
    int position_;
    const int length_;
    const AudioSample* data_;
    int priority_;
};


struct SoundContext {
    // Only three sounds will play at a time... hey, sound mixing's expensive!
    Buffer<ActiveSoundInfo, 3> active_sounds;

    const AudioSample* music_track = nullptr;
    int music_track_length = 0;
    int music_track_pos = 0;
    bool music_track_loop = false;
};


extern SoundContext snd_ctx;
