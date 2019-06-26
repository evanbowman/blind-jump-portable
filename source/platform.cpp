#include "platform.hpp"
#include "image_data.hpp"
#include <string.h>


using Tile = u32[16];
using TileBlock = Tile[256];


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


Screen& Platform::screen()
{
    return screen_;
}


Keyboard& Platform::keyboard()
{
    return keyboard_;
}
