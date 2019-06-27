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
    // Assumes 60 fps
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
    }
}


void Keyboard::poll()
{
    states_[action_1] = ~(*keys) & KEY_A;
    states_[action_2] = ~(*keys) & KEY_B;
    states_[start] = ~(*keys) & KEY_START;
    states_[right] = ~(*keys) & KEY_RIGHT;
    states_[left] = ~(*keys) & KEY_LEFT;
    states_[down] = ~(*keys) & KEY_DOWN;
    states_[up] = ~(*keys) & KEY_UP;
}



////////////////////////////////////////////////////////////////////////////////
// Sprite
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


constexpr u32 oam_count = 128;

ObjectPool<ObjectAttributes, oam_count> attribute_pool;


Sprite::~Sprite()
{
    if (data_) {
        attribute_pool.post((ObjectAttributes*)data_);
    }
}


bool Sprite::initialize(Size size, u32 keyframe)
{
    const auto oa = [this] {
        if (data_) {
            return (ObjectAttributes*)data_;
        } else {
            return attribute_pool.get();
        }
    }();

    if (not oa) {
        return false;
    }

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

    oa->attribute_0 = ATTR0_COLOR_16 | ATTR0_SQUARE;
    oa->attribute_1 = ATTR1_SIZE_32;
    oa->attribute_2 = 2 + keyframe * 16; //ATTR2_PALETTE(0) | OBJ_CHAR(0);

    data_ = oa;

    return true;
}


Sprite::Sprite() : data_(nullptr)
{
}


void Sprite::set_position(const Vec2<float>& position)
{
    const auto oa = (ObjectAttributes*)data_;
    oa->attribute_0 &= 0xff00;
    oa->attribute_0 |= (static_cast<int>(position.y)) & 0x00ff;
    oa->attribute_1 &= 0xfe00;
    oa->attribute_1 |= (static_cast<int>(position.x)) & 0x01ff;
}


const Vec2<float> Sprite::get_position() const
{
    const auto oa = (ObjectAttributes*)data_;
    return {
        static_cast<float>(oa->attribute_1 & 0x01ff),
        static_cast<float>(oa->attribute_0 & 0x00ff)
    };
}


void* Sprite::native_handle() const
{
    return data_;
}



////////////////////////////////////////////////////////////////////////////////
// Screen
////////////////////////////////////////////////////////////////////////////////


static u32 oam_write_index = 0;


static ObjectAttributes* const object_attribute_memory = {
    (ObjectAttributes*)0x07000000
};


static volatile u16* const scanline = (u16*)0x4000006;


Screen::Screen() : userdata_(nullptr)
{
    REG_DISPCNT = MODE_0 | OBJ_ENABLE | OBJ_MAP_1D;
}


void Screen::draw(const Sprite& spr)
{
    *(object_attribute_memory + oam_write_index) =
        *((ObjectAttributes*)spr.native_handle());

    oam_write_index += 1;
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


#include "spritesheet.h"


void load_sprite_data()
{
#define MEM_TILE      ((TileBlock*)0x6000000 )
#define MEM_PALETTE   ((u16*)(0x05000200))

    memcpy((void*)MEM_PALETTE, spritesheetPal, spritesheetPalLen);
    memcpy((void*)&MEM_TILE[4][1], spritesheetTiles, spritesheetTilesLen);
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
