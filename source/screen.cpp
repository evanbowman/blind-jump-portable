#include "screen.hpp"
#include <string.h>


#define REG_DISPCNT    *(u32*)0x4000000
#define MODE_0 0x0
#define OBJ_MAP_1D 0x40
#define OBJ_ENABLE 0x1000


Screen::Screen() :
    // Don't bother setting the state pointer. State is mainly here for other
    // desktop targets, which require the creation of a non-global
    // window. Seeing as we're an embedded system, and there're no windows, and
    // only one screen, global state is fine.
    userdata_(nullptr)
{
    REG_DISPCNT = MODE_0 | OBJ_MAP_1D | OBJ_ENABLE;
}


void Screen::draw(const Sprite& spr)
{
    // Nothing to do, the GBA draws sprites automatically.
}


static volatile u16* const scanline = (u16*)0x4000006;


void Screen::clear()
{
    while (*scanline < 160); // VSync
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
