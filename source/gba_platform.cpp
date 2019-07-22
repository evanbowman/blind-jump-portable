
////////////////////////////////////////////////////////////////////////////////
//
//
// Gameboy Advance Platform
//
//
////////////////////////////////////////////////////////////////////////////////

#ifdef __GBA__

#include "platform.hpp"
#include "random.hpp"
#include <string.h>

// FIXME: I'm relying on devkit ARM right now for handling
// interrupts. But it would be more educational to set this stuff up
// on my own!
#include "/opt/devkitpro/libgba/include/gba_interrupt.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#include "/opt/devkitpro/libgba/include/gba_systemcalls.h"
#pragma GCC diagnostic pop

#define REG_DISPCNT *(u32*)0x4000000
#define MODE_0 0x0
#define OBJ_MAP_1D 0x40
#define OBJ_ENABLE 0x1000

#define KEY_A 0x0001
#define KEY_B 0x0002
#define KEY_SELECT 0x0004
#define KEY_START 0x0008
#define KEY_RIGHT 0x0010
#define KEY_LEFT 0x0020
#define KEY_UP 0x0040
#define KEY_DOWN 0x0080
#define KEY_R 0x0100
#define KEY_L 0x0200

using HardwareTile = u32[16];
using TileBlock = HardwareTile[256];
using ScreenBlock = u16[1024];

#define MEM_TILE ((TileBlock*)0x6000000)
#define MEM_PALETTE ((u16*)(0x05000200))
#define MEM_BG_PALETTE ((u16*)(0x05000000))
#define MEM_SCREENBLOCKS ((ScreenBlock*)0x6000000)

#define ATTR2_PALBANK_MASK 0xF000
#define ATTR2_PALBANK_SHIFT 12
#define ATTR2_PALBANK(n) ((n) << ATTR2_PALBANK_SHIFT)

////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


DeltaClock::DeltaClock() : impl_(nullptr)
{
}


static volatile u16* const scanline = (u16*)0x4000006;


Microseconds DeltaClock::reset()
{
    // (1 second / 60 frames) x (1,000,000 microseconds / 1 second) =
    // 16,666.6...
    constexpr Microseconds fixed_step = 16667;
    return fixed_step;
}


DeltaClock::~DeltaClock()
{
}


////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////


static volatile u32* keys = (volatile u32*)0x04000130;


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
    states_[alt_1] = ~(*keys) & KEY_L;
    states_[alt_2] = ~(*keys) & KEY_R;
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
    (ObjectAttributes*)0x07000000};


#define OBJ_SHAPE(m) ((m) << 14)
#define ATTR0_COLOR_16 (0 << 13)
#define ATTR0_COLOR_256 (1 << 13)
#define ATTR0_SQUARE OBJ_SHAPE(0)
#define ATTR0_TALL OBJ_SHAPE(2)
#define ATTR0_WIDE OBJ_SHAPE(1)
#define ATTR0_BLEND 0x0400
#define ATTR1_SIZE_16 (1 << 14)
#define ATTR1_SIZE_32 (2 << 14)
#define ATTR1_SIZE_64 (3 << 14)
#define ATTR2_PALETTE(n) ((n) << 12)
#define OBJ_CHAR(m) ((m)&0x03ff)
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800


static volatile u16* bg0_control = (volatile u16*)0x4000008;
static volatile u16* bg1_control = (volatile u16*)0x400000a;
// static volatile u16* bg2_control = (volatile u16*)0x400000c;
// static volatile u16* bg3_control = (volatile u16*)0x400000e;


static volatile short* bg0_x_scroll = (volatile short*)0x4000010;
static volatile short* bg0_y_scroll = (volatile short*)0x4000012;
static volatile short* bg1_x_scroll = (volatile short*)0x4000014;
static volatile short* bg1_y_scroll = (volatile short*)0x4000016;
// static volatile short* bg2_x_scroll = (volatile short*)0x4000018;
// static volatile short* bg2_y_scroll = (volatile short*)0x400001a;
// static volatile short* bg3_x_scroll = (volatile short*)0x400001c;
// static volatile short* bg3_y_scroll = (volatile short*)0x400001e;


static volatile u16* reg_blendcnt = (volatile u16*)0x04000050;
static volatile u16* reg_blendalpha = (volatile u16*)0x04000052;

#define BLD_BUILD(top, bot, mode)                                              \
    ((((bot)&63) << 8) | (((mode)&3) << 6) | ((top)&63))
#define BLD_OBJ 0x0010
#define BLD_BG0 0x0001
#define BLD_BG1 0x0002
#define BLDA_BUILD(eva, evb) (((eva)&31) | (((evb)&31) << 8))


class Color {
public:
    Color(u8 r, u8 g, u8 b) : r_(r), g_(g), b_(b)
    {
    }

    inline u16 bgr_hex_555() const
    {
        return (r_) + ((g_) << 5) + ((b_) << 10);
    }

    static Color from_bgr_hex_555(u16 val)
    {
        return {
            u8(0x1F & val), u8((0x3E0 & val) >> 5), u8((0x7C00 & val) >> 10)};
    }

    u8 r_;
    u8 g_;
    u8 b_;
};


Screen::Screen() : userdata_(nullptr)
{
    REG_DISPCNT = MODE_0 | OBJ_ENABLE | OBJ_MAP_1D | BG0_ENABLE | BG1_ENABLE;

    *reg_blendcnt = BLD_BUILD(BLD_OBJ, BLD_BG0 | BLD_BG1, 0);

    *reg_blendalpha = BLDA_BUILD(0x40 / 8, 0x40 / 8);


    view_.set_size(this->size().cast<Float>());
}


static u32 last_oam_write_index = 0;
static u32 oam_write_index = 0;


using PaletteBank = int;
constexpr PaletteBank available_palettes = 3;

static PaletteBank palette_counter = available_palettes;


// Perform a color mix between the spritesheet palette bank (bank zero), and
// return the palette bank where the resulting mixture is stored. We can only
// display 12 mixed colors at a time, because the first four banks are in use.
static PaletteBank color_mix(const Color& c, float amount)
{
    if (palette_counter == 15) {
        return 0; // Exhausted all the palettes that we have for effects.
    }
    for (int i = 0; i < 16; ++i) {
        auto from = Color::from_bgr_hex_555(MEM_PALETTE[i]);
        const u32 index = 16 * (palette_counter + 1) + i;
        MEM_PALETTE[index] = Color(interpolate(c.r_, from.r_, amount),
                                   interpolate(c.g_, from.g_, amount),
                                   interpolate(c.b_, from.b_, amount))
                                 .bgr_hex_555();
    }
    return ++palette_counter;
}


const Color& real_color(ColorConstant k)
{
    switch (k) {
    case ColorConstant::spanish_crimson:
        static const Color spn_crimson(29, 3, 11);
        return spn_crimson;

    case ColorConstant::electric_blue:
        static const Color el_blue(9, 31, 31);
        return el_blue;

    case ColorConstant::coquelicot:
        static const Color coquelicot(30, 7, 1);
        return coquelicot;

    default:
    case ColorConstant::null:
    case ColorConstant::rich_black:
        static const Color rich_black(0, 0, 2);
        return rich_black;
    }
}


void Screen::draw(const Sprite& spr)
{
    const auto pb = [&]() -> PaletteBank {
        const auto& mix = spr.get_mix();
        if (mix.color_ not_eq ColorConstant::null) {
            if (const auto pal_bank =
                    color_mix(real_color(mix.color_), mix.amount_)) {
                return ATTR2_PALBANK(pal_bank);
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }();

    auto draw_sprite = [&](int tex_off, int x_off, int scale) {
        if (oam_write_index == oam_count) {
            return;
        }
        const auto position = spr.get_position().cast<s32>() - spr.get_origin();
        const auto view_center = view_.get_center().cast<s32>();
        auto oa = object_attribute_memory + oam_write_index;
        if (spr.get_alpha() not_eq Sprite::Alpha::translucent) {
            oa->attribute_0 = ATTR0_COLOR_16 | ATTR0_TALL;
        } else {
            oa->attribute_0 = ATTR0_COLOR_16 | ATTR0_TALL | ATTR0_BLEND;
        }
        oa->attribute_1 = ATTR1_SIZE_32;
        const auto& flip = spr.get_flip();
        if (flip.y) {
            oa->attribute_1 |= (1 << 13);
        }
        if (flip.x) {
            oa->attribute_1 |= (1 << 12);
        }
        const auto abs_position = position - view_center;
        oa->attribute_0 &= 0xff00;
        oa->attribute_0 |= abs_position.y & 0x00ff;
        oa->attribute_1 &= 0xfe00;
        oa->attribute_1 |= (abs_position.x + x_off) & 0x01ff;
        oa->attribute_2 = 2 + spr.get_texture_index() * scale + tex_off;
        oa->attribute_2 |= pb;
        oam_write_index += 1;
    };

    switch (spr.get_size()) {
    case Sprite::Size::w32_h32:
        // In order to fit the spritesheet into VRAM, the game draws
        // sprites in 32x16 pixel chunks, although several sprites are
        // really 32x32. 32x16 is easy to meta-tile for 1D texture
        // mapping, and a 32x32 sprite can be represented as two 32x16
        // sprites. If all sprites were 32x32, the spritesheet would
        // not fit into the gameboy advance's video memory. 16x16
        // would be even more compact, but would be inconvenient to
        // work with from a art/drawing perspective. Maybe I'll write
        // a script to reorganize the spritesheet into a Nx16 strip,
        // and metatile as 2x2 gba tiles... someday.
        draw_sprite(0, 0, 16);
        draw_sprite(8, 16, 16);
        break;

    case Sprite::Size::w16_h32:
        draw_sprite(0, 0, 8);
        break;
    }
}


void Screen::clear()
{
    // VSync
    VBlankIntrWait();
}


void Screen::display()
{
    // The Sprites are technically already displayed, so all we really
    // need to do is turn off the sprites in the table if the sprite
    // draw count dropped since the last draw batch.
    for (u32 i = oam_write_index; i < last_oam_write_index; ++i) {
        object_attribute_memory[i].attribute_0 |= attr0_mask::disabled;
    }

    last_oam_write_index = oam_write_index;
    oam_write_index = 0;
    palette_counter = available_palettes;

    auto view_offset = view_.get_center().cast<s32>();
    *bg0_x_scroll = view_offset.x;
    *bg0_y_scroll = view_offset.y;
    *bg1_x_scroll = view_offset.x * 0.3f;
    *bg1_y_scroll = view_offset.y * 0.3f;
}


Vec2<u32> Screen::size() const
{
    static const Vec2<u32> gba_widescreen{240, 160};
    return gba_widescreen;
}


////////////////////////////////////////////////////////////////////////////////
// Platform
////////////////////////////////////////////////////////////////////////////////


#include "bgr_spritesheet.h"
#include "bgr_tilesheet.h"


// NOTE: ScreenBlock layout:

// The first screenblock starts at 8, and the game uses four
// screenblocks for drawing the background maps.  The whole first
// charblock is used up by the tileset image.


static void set_tile(u16 x, u16 y, u16 tile_id)
{
    // NOTE: The game's tiles are 32x24px in size. GBA tiles are each
    // 8x8. To further complicate things, the GBA's VRAM is
    // partitioned into 32x32 tile screenblocks, so some 32x24px tiles
    // cross over screenblocks in the vertical direction, and then the
    // y-offset is one-tile-greater in the lower quadrants.

    auto ref = [](u16 x_, u16 y_) { return x_ * 4 + y_ * 32 * 3; };

    // Tiles at y=10 need to jump across a gap between screen blocks.
    if (y == 10 and x > 7) {
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[9][i + ref(x % 8, y)] = tile_id * 12 + i;
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[9][i + ref(x % 8, y) + 32] = tile_id * 12 + i + 4;
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[11][i + ref(x % 8, 0)] = tile_id * 12 + i + 8;
        }
        return;
    } else if (y == 10) {
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[8][i + ref(x, y)] = tile_id * 12 + i;
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[8][i + ref(x, y) + 32] = tile_id * 12 + i + 4;
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[10][i + ref(x, 0)] = tile_id * 12 + i + 8;
        }
        return;
    }

    auto screen_block = [&]() -> u16 {
        if (x > 7 and y > 9) {
            x %= 8;
            y %= 10;
            return 11;
        } else if (y > 9) {
            y %= 10;
            return 10;
        } else if (x > 7) {
            x %= 8;
            return 9;
        } else {
            return 8;
        }
    }();

    if (screen_block == 10 or screen_block == 11) {
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y - 1) + 32] =
                tile_id * 12 + i;
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y - 1) + 64] =
                tile_id * 12 + i + 4;
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y - 1) + 96] =
                tile_id * 12 + i + 8;
        }
    } else {
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y)] = tile_id * 12 + i;
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y) + 32] =
                tile_id * 12 + i + 4;
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y) + 64] =
                tile_id * 12 + i + 8;
        }
    }
}


void Platform::push_map(const TileMap& map)
{
    for (u32 i = 0; i < TileMap::width; ++i) {
        for (u32 j = 0; j < TileMap::height; ++j) {
            set_tile(i, j, static_cast<u16>(map.get_tile(i, j)));
        }
    }

    // Note: we want to reload the space background so that it looks
    // different with each new map.
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 32; ++j) {
            if (random_choice<8>()) {
                MEM_SCREENBLOCKS[12][i + j * 32] = 67;
            } else {
                if (random_choice<2>()) {
                    MEM_SCREENBLOCKS[12][i + j * 32] = 70;
                } else {
                    MEM_SCREENBLOCKS[12][i + j * 32] = 71;
                }
            }
        }
    }
}

#define BG_CBB_MASK 0x000C
#define BG_CBB_SHIFT 2
#define BG_CBB(n) ((n) << BG_CBB_SHIFT)

#define BG_SBB_MASK 0x1F00
#define BG_SBB_SHIFT 8
#define BG_SBB(n) ((n) << BG_SBB_SHIFT)

#define BG_SIZE_MASK 0xC000
#define BG_SIZE_SHIFT 14
#define BG_SIZE(n) ((n) << BG_SIZE_SHIFT)

#define BG_REG_32x32 0      //!< reg bg, 32x32 (256x256 px)
#define BG_REG_64x32 0x4000 //!< reg bg, 64x32 (512x256 px)
#define BG_REG_32x64 0x8000 //!< reg bg, 32x64 (256x512 px)
#define BG_REG_64x64 0xC000 //!< reg bg, 64x64 (512x512 px)


static void load_sprite_data()
{
    memcpy((void*)MEM_PALETTE, bgr_spritesheetPal, bgr_spritesheetPalLen);

    // NOTE: There are four tile blocks, so index four points to the
    // end of the tile memory.
    memcpy(
        (void*)&MEM_TILE[4][1], bgr_spritesheetTiles, bgr_spritesheetTilesLen);

    memcpy((void*)MEM_BG_PALETTE, bgr_tilesheetPal, bgr_tilesheetPalLen);
    memcpy((void*)&MEM_TILE[0][0], bgr_tilesheetTiles, bgr_tilesheetTilesLen);

    *bg0_control = BG_SBB(8) | BG_REG_64x64;
    //        0xC800; // 64x64, 0x0100 for 32x32
    *bg1_control = BG_SBB(12);
}


void Screen::fade(float amount, ColorConstant k)
{
    const auto& c = real_color(k);
    // To do a screen fade, blend black into the palettes.
    for (int i = 0; i < 16; ++i) {
        auto from = Color::from_bgr_hex_555(bgr_spritesheetPal[i]);
        MEM_PALETTE[i] = Color(interpolate(c.r_, from.r_, amount),
                               interpolate(c.g_, from.g_, amount),
                               interpolate(c.b_, from.b_, amount))
                             .bgr_hex_555();
    }
    for (int i = 0; i < 16; ++i) {
        auto from = Color::from_bgr_hex_555(bgr_tilesheetPal[i]);
        MEM_BG_PALETTE[i] = Color(interpolate(c.r_, from.r_, amount),
                                  interpolate(c.g_, from.g_, amount),
                                  interpolate(c.b_, from.b_, amount))
                                .bgr_hex_555();
    }
}


void Platform::sleep(u32 frames)
{
    while (frames--) {
        VBlankIntrWait();
    }
}


bool Platform::is_running() const
{
    // Unlike the windowed desktop platform, as long as the device is
    // powered on, the game is running.
    return true;
}


static byte* const sram = (byte*)0x0E000000;


static bool
flash_byteverify(void* in_dst, const void* in_src, unsigned int length)
{
    unsigned char* src = (unsigned char*)in_src;
    unsigned char* dst = (unsigned char*)in_dst;

    for (; length > 0; length--) {

        if (*dst++ != *src++)
            return true;
    }
    return false;
}


static void
flash_bytecpy(void* in_dst, const void* in_src, unsigned int length, bool write)
{
    unsigned char* src = (unsigned char*)in_src;
    unsigned char* dst = (unsigned char*)in_dst;

    for (; length > 0; length--) {
        if (write) {
            *(vu8*)0x0E005555 = 0xAA;
            *(vu8*)0x0E002AAA = 0x55;
            *(vu8*)0x0E005555 = 0xA0;
        }
        *dst++ = *src++;
    }
}


static void set_flash_bank(u32 bankID)
{
    if (bankID < 2) {
        *(vu8*)0x0E005555 = 0xAA;
        *(vu8*)0x0E002AAA = 0x55;
        *(vu8*)0x0E005555 = 0xB0;
        *(vu8*)0x0E000000 = bankID;
    }
}


template <typename T> static bool flash_save(const T& obj, u32 flash_offset)
{
    if ((u32)flash_offset >= 0x10000) {
        set_flash_bank(1);
    } else {
        set_flash_bank(0);
    }

    static_assert(std::is_standard_layout<T>());

    flash_bytecpy((void*)(sram + flash_offset), &obj, sizeof(T), true);

    if (flash_byteverify((void*)(sram + flash_offset), &obj, sizeof(T))) {
        return true;
    }
    return false;
}


template <typename T> static T flash_load(u32 flash_offset)
{
    if (flash_offset >= 0x10000) {
        set_flash_bank(1);
    } else {
        set_flash_bank(0);
    }

    static_assert(std::is_standard_layout<T>());

    T result;
    flash_bytecpy(
        &result, (const void*)(sram + flash_offset), sizeof(T), false);

    return result;
}


// FIXME: Lets be nice to the flash an not write to the same
// memory location every single time! What about a list? Each new
// save will have a unique id. On startup, scan through memory
// until you reach the highest unique id. Then start writing new
// saves at that point.

bool Platform::write_save(const SaveData& data)
{
    if (not flash_save(data, 0)) {
        return false;
    }

    // Sanity check, that writing the save file succeeded.
    const auto d = flash_load<SaveData>(0);
    if (d.magic_ not_eq SaveData::magic_val) {
        return false;
    }
    return true;
}


std::optional<SaveData> Platform::read_save()
{
    auto sd = flash_load<SaveData>(0);
    if (sd.magic_ == SaveData::magic_val) {
        return sd;
    }
    return {};
}


Platform::Platform()
{
    irqInit();
    irqEnable(IRQ_VBLANK);

    load_sprite_data();
}


#endif // __GBA__
