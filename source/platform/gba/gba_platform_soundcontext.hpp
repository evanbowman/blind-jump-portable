#pragma once


#include "memory/buffer.hpp"


using VolumeScaleLUT = std::array<s8, 256>;


using AudioSample = s8;


struct ActiveSoundInfo {
    s32 position_;
    const s32 length_;
    const AudioSample* data_;
    s32 priority_;
    const VolumeScaleLUT* l_volume_lut_;
    const VolumeScaleLUT* r_volume_lut_;
};


struct SoundContext {
    // Only three sounds will play at a time... hey, sound mixing's expensive!
    Buffer<ActiveSoundInfo, 3> active_sounds;

    const AudioSample* music_track = nullptr;
    s32 music_track_length = 0;
    s32 music_track_pos = 0;
};
