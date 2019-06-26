#include "platform.hpp"
#include <string.h>


Screen& Platform::screen()
{
    return screen_;
}


Keyboard& Platform::keyboard()
{
    return keyboard_;
}


using Tile = u32[16];
using TileBlock = Tile[256];


#include "image_data.hpp"


void load_sprite_data()
{
#define MEM_TILE      ((TileBlock*)0x6000000 )
#define MEM_PALETTE   ((u16*)(0x05000200))

    memcpy((void*)MEM_PALETTE, spritePal, 12);
    memcpy((void*)&MEM_TILE[4][1], spriteTiles, 256);
}


Platform::Platform()
{
    load_sprite_data();
}
