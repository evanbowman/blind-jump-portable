#include "platform.hpp"
#include <bitset>
#include <string.h>
#include "pool.hpp"



////////////////////////////////////////////////////////////////////////////////
//
//
// Gameboy Advance Platform
//
//
////////////////////////////////////////////////////////////////////////////////

#ifdef __GBA__

#define REG_DISPCNT    *(u32*)0x4000000
#define MODE_0 0x0
#define OBJ_MAP_1D 0x40
#define OBJ_ENABLE 0x1000

#define KEY_A        0x0001
#define KEY_B        0x0002
#define KEY_SELECT   0x0004
#define KEY_START    0x0008
#define KEY_RIGHT    0x0010
#define KEY_LEFT     0x0020
#define KEY_UP       0x0040
#define KEY_DOWN     0x0080
#define KEY_R        0x0100
#define KEY_L        0x0200


#if	defined	( __thumb__ )
#define	SystemCall(Number)	 __asm ("SWI	  "#Number"\n" :::  "r0", "r1", "r2", "r3")
#else
#define	SystemCall(Number)	 __asm ("SWI	  "#Number"	<< 16\n" :::"r0", "r1", "r2", "r3")
#endif



////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


DeltaClock::DeltaClock() : impl_(nullptr)
{

}


Microseconds DeltaClock::reset()
{
    // (1 second / 60 frames) x (1,000,000 microseconds / 1 second) = 16,666.6...
    constexpr Microseconds fixed_step = 16667;
    return fixed_step;
}



////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////


static volatile u32* keys = (volatile u32*)0x04000130;


Keyboard::Keyboard()
{
    for (int i = 0; i < Key::count; ++i) {
        states_[i] = false;
        prev_[i] = false;
    }
}


void Keyboard::poll()
{
    for (size_t i = 0; i < Key::count; ++i) {
        prev_[i] = states_[i];
    }
    states_[action_1] = ~(*keys) & KEY_A;
    states_[action_2] = ~(*keys) & KEY_B;
    states_[start] = ~(*keys) & KEY_START;
    states_[right] = ~(*keys) & KEY_RIGHT;
    states_[left] = ~(*keys) & KEY_LEFT;
    states_[down] = ~(*keys) & KEY_DOWN;
    states_[up] = ~(*keys) & KEY_UP;
}



////////////////////////////////////////////////////////////////////////////////
// Screen
////////////////////////////////////////////////////////////////////////////////


struct alignas(4) ObjectAttributes {
    u16 attribute_0;
    u16 attribute_1;
    u16 attribute_2;
    u16 pad;
};


namespace attr0_mask {
    constexpr u16 disabled{2 << 8};
}


constexpr u32 oam_count = Screen::sprite_limit;


static ObjectAttributes* const object_attribute_memory = {
    (ObjectAttributes*)0x07000000
};


static volatile u16* const scanline = (u16*)0x4000006;


#define OBJ_SHAPE(m)		((m)<<14)
#define ATTR0_COLOR_16			(0<<13)
#define ATTR0_COLOR_256			(1<<13)
#define ATTR0_SQUARE		OBJ_SHAPE(0)
#define ATTR0_TALL		OBJ_SHAPE(2)
#define ATTR0_WIDE		OBJ_SHAPE(1)
#define ATTR1_SIZE_16         (1<<14)
#define ATTR1_SIZE_32         (2<<14)
#define ATTR1_SIZE_64         (3<<14)
#define ATTR2_PALETTE(n)      ((n)<<12)
#define OBJ_CHAR(m)		((m)&0x03ff)


Screen::Screen() : userdata_(nullptr)
{
    REG_DISPCNT = MODE_0 | OBJ_ENABLE | OBJ_MAP_1D;

    // TODO: We can probably preload some object attributes here, seeing as we
    // know that the sprites will be 32x32 square with 16bit color. Then the
    // draw() function won't have to initialize the first two attributes each
    // time, although it's probably not _that_ expensive. Remember to un-disable
    // attr0 though.

    view_.set_size(this->size().cast<Float>());
}


static u32 oam_write_index = 0;


void Screen::draw(const Sprite& spr)
{
    const auto& view_center = view_.get_center();
    if (oam_write_index < oam_count) {
        auto oa = object_attribute_memory + oam_write_index;
        auto position = spr.get_position().cast<s32>() - view_center.cast<s32>();
        oa->attribute_0 = ATTR0_COLOR_16 | ATTR0_SQUARE;
        oa->attribute_1 = ATTR1_SIZE_32;
        oa->attribute_0 &= 0xff00;
        oa->attribute_0 |= position.y & 0x00ff;
        oa->attribute_1 &= 0xfe00;
        oa->attribute_1 |= position.x & 0x01ff;
        oa->attribute_2 = 2 + spr.get_texture_index() * 16;
        oam_write_index += 1;
    }
}


void Screen::clear()
{
    while (*scanline < 160); // VSync

    for (u32 i = 0; i < oam_write_index; ++i) {
        object_attribute_memory[i].attribute_0 |= attr0_mask::disabled;
    }

    oam_write_index = 0;
}


void Screen::display()
{
    // Nothing to do here.
}


const Vec2<u32>& Screen::size() const
{
    static const Vec2<u32> gba_widescreen {240, 160};
    return gba_widescreen;
}



////////////////////////////////////////////////////////////////////////////////
// Platform
////////////////////////////////////////////////////////////////////////////////


using Tile = u32[16];
using TileBlock = Tile[256];


#include "bgr_spritesheet.h"


void load_sprite_data()
{
#define MEM_TILE      ((TileBlock*)0x6000000 )
#define MEM_PALETTE   ((u16*)(0x05000200))

    memcpy((void*)MEM_PALETTE, bgr_spritesheetPal, bgr_spritesheetPalLen);
    memcpy((void*)&MEM_TILE[4][1], bgr_spritesheetTiles, bgr_spritesheetTilesLen);
}


Platform::Platform()
{
    load_sprite_data();
}


#else

////////////////////////////////////////////////////////////////////////////////
//
//
// Desktop Platform
//
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// Sprite
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// Screen
////////////////////////////////////////////////////////////////////////////////


#endif
