#include "sprite.hpp"
#include <string.h>
#include <bitset>



struct alignas(4) ObjectAttributes {
    u16 attribute_0;
    u16 attribute_1;
    u16 attribute_2;
    u16 pad;
};


static volatile ObjectAttributes* const object_attribute_memory = {
    (ObjectAttributes*)0x07000000
};


// Note: GBA supports 128 sprites, and we need to keep track of which
// ones we've used.
static std::bitset<128> oam_freespace;
static bool freespace_has_init = []{ oam_freespace.set(); return true; }();


Sprite::~Sprite()
{
    if (data_) {
        const auto offset = (ObjectAttributes*)data_ - object_attribute_memory;
        oam_freespace[offset] = true;
    }
}


bool Sprite::initialize(int x)
{
    const auto oa = []() -> volatile ObjectAttributes* {
        for (u32 i = 0; i < oam_freespace.size(); ++i) {
            if (oam_freespace[i]) {
                oam_freespace[i] = false;
                return &object_attribute_memory[i];
            }
        }
        return nullptr;
    }();

    if (not oa) {
        return false;
    }

    oa->attribute_0 = 0x2032; // 8bpp tiles, SQUARE shape, at y coord 50
    oa->attribute_1 = 0x4000 | (0x1FF & x); // 16x16 size when using the SQUARE shape
    oa->attribute_2 = 2;      // Start at the first tile in tile

    data_ = oa;

    return true;
}


Sprite::Sprite() : data_(nullptr)
{

}


void Sprite::set_position(Vec2<float>& position)
{
    const auto oa = (ObjectAttributes*)data_;
    oa->attribute_0;
}


const Vec2<float>& Sprite::get_position() const
{

}
