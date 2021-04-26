// clang-format off

// This file was generated by the project's image build pipeline in
// CMakeLists.txt. I have no idea why cmake outputs semicolons between the
// entries in the IMAGE_INCLUDES list variable... maybe some special formatting
// character that I don't know about... or maybe cmake uses ; internally as a
// delimiter... but for now, the cmake script outputs the semicolons on a new
// line, commented out, to suppress the resulting errors.
//
// If you try to manually edit source/platform/gba/images.cpp, the file may be
// overwritten if you have the GBA_AUTOBUILD_IMG option in the cmake environment
// turned on.



#include "data/spritesheet_intro_clouds.h"
//;
#include "data/spritesheet_intro_cutscene.h"
//;
#include "data/spritesheet.h"
//;
#include "data/spritesheet2.h"
//;
#include "data/spritesheet3.h"
//;
#include "data/spritesheet4.h"
//;
#include "data/spritesheet_boss0.h"
//;
#include "data/spritesheet_boss1.h"
//;
#include "data/spritesheet_boss2.h"
//;
#include "data/spritesheet_boss2_done.h"
//;
#include "data/spritesheet_boss2_mutate.h"
//;
#include "data/spritesheet_boss2_final.h"
//;
#include "data/spritesheet_boss3.h"
//;
#include "data/spritesheet_launch_anim.h"
//;
#include "data/title_1_flattened.h"
//;
#include "data/title_2_flattened.h"
//;
#include "data/ending_scene_flattened.h"
//;
#include "data/ending_scene_2_flattened.h"
//;
#include "data/launch_flattened.h"
//;
#include "data/tilesheet_intro_cutscene_flattened.h"
//;
#include "data/tilesheet.h"
//;
#include "data/tilesheet2.h"
//;
#include "data/tilesheet3.h"
//;
#include "data/tilesheet4.h"
//;
#include "data/tilesheet_top.h"
//;
#include "data/tilesheet2_top.h"
//;
#include "data/tilesheet3_top.h"
//;
#include "data/tilesheet4_top.h"
//;
#include "data/overlay.h"
//;
#include "data/repl.h"
//;
#include "data/overlay_cutscene.h"
//;
#include "data/charset.h"
//;
#include "data/overlay_journal.h"
//;
#include "data/overlay_dialog.h"
//;
#include "data/death_text_english.h"
//;
#include "data/death_text_chinese.h"
//;
#include "data/death_text_russian.h"
//;
#include "data/old_poster_flattened.h"
//;
#include "data/old_poster_chinese_flattened.h"
//;
#include "data/blaster_info_flattened.h"
//;
#include "data/seed_packet_flattened.h"
//;
#include "data/seed_packet_chinese_flattened.h"
//;
#include "data/seed_packet_russian_flattened.h"
//;
#include "data/overlay_network_flattened.h"
//

struct TextureData {
    const char* name_;
    const unsigned int* tile_data_;
    const unsigned short* palette_data_;
    u32 tile_data_length_;
    u32 palette_data_length_;
};


#define STR(X) #X
#define TEXTURE_INFO(NAME)                                                     \
    {                                                                          \
        STR(NAME), NAME##Tiles, NAME##Pal, NAME##TilesLen, NAME##PalLen        \
    }


static const TextureData sprite_textures[] = {

    TEXTURE_INFO(spritesheet_intro_clouds),
//;
    TEXTURE_INFO(spritesheet_intro_cutscene),
//;
    TEXTURE_INFO(spritesheet),
//;
    TEXTURE_INFO(spritesheet2),
//;
    TEXTURE_INFO(spritesheet3),
//;
    TEXTURE_INFO(spritesheet4),
//;
    TEXTURE_INFO(spritesheet_boss0),
//;
    TEXTURE_INFO(spritesheet_boss1),
//;
    TEXTURE_INFO(spritesheet_boss2),
//;
    TEXTURE_INFO(spritesheet_boss2_done),
//;
    TEXTURE_INFO(spritesheet_boss2_mutate),
//;
    TEXTURE_INFO(spritesheet_boss2_final),
//;
    TEXTURE_INFO(spritesheet_boss3),
//;
    TEXTURE_INFO(spritesheet_launch_anim),
//
};


static const TextureData tile_textures[] = {

    TEXTURE_INFO(title_1_flattened),
//;
    TEXTURE_INFO(title_2_flattened),
//;
    TEXTURE_INFO(ending_scene_flattened),
//;
    TEXTURE_INFO(ending_scene_2_flattened),
//;
    TEXTURE_INFO(launch_flattened),
//;
    TEXTURE_INFO(tilesheet_intro_cutscene_flattened),
//;
    TEXTURE_INFO(tilesheet),
//;
    TEXTURE_INFO(tilesheet2),
//;
    TEXTURE_INFO(tilesheet3),
//;
    TEXTURE_INFO(tilesheet4),
//;
    TEXTURE_INFO(tilesheet_top),
//;
    TEXTURE_INFO(tilesheet2_top),
//;
    TEXTURE_INFO(tilesheet3_top),
//;
    TEXTURE_INFO(tilesheet4_top),
//
};


static const TextureData overlay_textures[] = {

    TEXTURE_INFO(overlay),
//;
    TEXTURE_INFO(repl),
//;
    TEXTURE_INFO(overlay_cutscene),
//;
    TEXTURE_INFO(charset),
//;
    TEXTURE_INFO(overlay_journal),
//;
    TEXTURE_INFO(overlay_dialog),
//;
    TEXTURE_INFO(death_text_english),
//;
    TEXTURE_INFO(death_text_chinese),
//;
    TEXTURE_INFO(death_text_russian),
//;
    TEXTURE_INFO(old_poster_flattened),
//;
    TEXTURE_INFO(old_poster_chinese_flattened),
//;
    TEXTURE_INFO(blaster_info_flattened),
//;
    TEXTURE_INFO(seed_packet_flattened),
//;
    TEXTURE_INFO(seed_packet_chinese_flattened),
//;
    TEXTURE_INFO(seed_packet_russian_flattened),
//;
    TEXTURE_INFO(overlay_network_flattened),
//
};

// clang-format on
