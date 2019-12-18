
////////////////////////////////////////////////////////////////////////////////
//
//
// Gameboy Advance Platform
//
//
////////////////////////////////////////////////////////////////////////////////

#ifdef __GBA__

#include "util.hpp"
#include "platform.hpp"
#include "number/random.hpp"
#include <string.h>


void start(Platform&);

int main()
{
    Platform pf;
    start(pf);
}


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


void Platform::Keyboard::poll()
{
    for (size_t i = 0; i < int(Key::count); ++i) {
        prev_[i] = states_[i];
    }
    states_[int(Key::action_1)] = ~(*keys) & KEY_A;
    states_[int(Key::action_2)] = ~(*keys) & KEY_B;
    states_[int(Key::start)] = ~(*keys) & KEY_START;
    states_[int(Key::right)] = ~(*keys) & KEY_RIGHT;
    states_[int(Key::left)] = ~(*keys) & KEY_LEFT;
    states_[int(Key::down)] = ~(*keys) & KEY_DOWN;
    states_[int(Key::up)] = ~(*keys) & KEY_UP;
    states_[int(Key::alt_1)] = ~(*keys) & KEY_L;
    states_[int(Key::alt_2)] = ~(*keys) & KEY_R;
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


constexpr u32 oam_count = Platform::Screen::sprite_limit;


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


Platform::Screen::Screen() : userdata_(nullptr)
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
static PaletteBank color_mix(const Color& c, u8 amount)
{
    if (UNLIKELY(palette_counter == 15)) {
        return 0; // Exhausted all the palettes that we have for effects.
    }

    for (int i = 0; i < 16; ++i) {
        auto from = Color::from_bgr_hex_555(MEM_PALETTE[i]);
        const u32 index = 16 * (palette_counter + 1) + i;
        MEM_PALETTE[index] = Color(fast_interpolate(c.r_, from.r_, amount),
                                   fast_interpolate(c.g_, from.g_, amount),
                                   fast_interpolate(c.b_, from.b_, amount))
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


void Platform::Screen::draw(const Sprite& spr)
{
    const auto pb = [&]() -> PaletteBank {
        const auto& mix = spr.get_mix();
        if (UNLIKELY(mix.color_ not_eq ColorConstant::null)) {
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
        if (UNLIKELY(oam_write_index == oam_count)) {
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


Buffer<Platform::Task*, 16> task_queue_;


void Platform::push_task(Task* task)
{
    task->complete_ = false;
    task->running_ = true;
    task_queue_.push_back(task);
}


void Platform::Screen::clear()
{
    // On the GBA, we don't have real threads, so run tasks prior to the vsync,
    // so any updates are least likely to cause tearing.
    for (auto it = task_queue_.begin(); it not_eq task_queue_.end();) {
        (*it)->run();
        if ((*it)->complete()) {
            (*it)->running_ = false;
            task_queue_.erase(it);
        } else {
            ++it;
        }
    }

    // VSync
    VBlankIntrWait();
}


void Platform::Screen::display()
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


Vec2<u32> Platform::Screen::size() const
{
    static const Vec2<u32> gba_widescreen{240, 160};
    return gba_widescreen;
}


////////////////////////////////////////////////////////////////////////////////
// Platform
////////////////////////////////////////////////////////////////////////////////


#include "graphics/bgr_spritesheet.h"
#include "graphics/bgr_tilesheet.h"


// NOTE: ScreenBlock layout:

// The first screenblock starts at 8, and the game uses four
// screenblocks for drawing the background maps.  The whole first
// charblock is used up by the tileset image.


COLD static void set_tile(u16 x, u16 y, u16 tile_id)
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


COLD void Platform::push_map(const TileMap& map)
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


COLD static void load_sprite_data()
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


static u8 last_fade_amt;


void Platform::Screen::fade(float amount, ColorConstant k)
{
    const u8 amt = amount * 255;

    if (amt == last_fade_amt) {
        return;
    }

    last_fade_amt = amt;

    const auto& c = real_color(k);
    // To do a screen fade, blend color into the palettes.
    auto blend = [&](const Color& from) {
        return [&] {
            switch (amt) {
            case 0: return from;
            case 255: return c;
            default:
                return Color(fast_interpolate(c.r_, from.r_, amt),
                             fast_interpolate(c.g_, from.g_, amt),
                             fast_interpolate(c.b_, from.b_, amt));
            }
        }().bgr_hex_555();
    };

    for (int i = 0; i < 16; ++i) {
        auto from = Color::from_bgr_hex_555(bgr_spritesheetPal[i]);
        MEM_PALETTE[i] = blend(from);
    }
    for (int i = 0; i < 16; ++i) {
        auto from = Color::from_bgr_hex_555(bgr_tilesheetPal[i]);
        MEM_BG_PALETTE[i] = blend(from);
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


template <typename T>
COLD static bool flash_save(const T& obj, u32 flash_offset)
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


template <typename T> COLD static T flash_load(u32 flash_offset)
{
    if (flash_offset >= 0x10000) {
        set_flash_bank(1);
    } else {
        set_flash_bank(0);
    }

    static_assert(std::is_standard_layout<T>());

    T result;
    for (u32 i = 0; i < sizeof(result); ++i) {
        ((u8*)&result)[i] = 0;
    }

    flash_bytecpy(
        &result, (const void*)(sram + flash_offset), sizeof(T), false);

    return result;
}


// FIXME: Lets be nice to the flash an not write to the same memory
// location every single time! What about a list? Each new save will
// have a unique id. On startup, scan through memory until you reach
// the highest unique id. Then start writing new saves at that
// point. NOTE: My everdrive uses SRAM for Flash writes anyway, so
// it's probably not going to wear out, but I like to pretend that I'm
// developing a real gba game.

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


void SynchronizedBase::init(Platform& pf)
{
}


void SynchronizedBase::lock()
{
}


void SynchronizedBase::unlock()
{
}


SynchronizedBase::~SynchronizedBase()
{
}


////////////////////////////////////////////////////////////////////////////////
// Logger
////////////////////////////////////////////////////////////////////////////////


// NOTE: SaveData goes first into flash memory, followed by the game's
// logs.
static u32 log_write_loc = sizeof(SaveData);


Platform::Logger::Logger()
{
}


void Platform::Logger::log(Logger::Severity level, const char* msg)
{
    std::array<char, 1024> buffer;

    buffer[0] = '[';
    buffer[2] = ']';

    switch (level) {
    case Severity::info:
        buffer[1] = 'i';
        break;

    case Severity::warning:
        buffer[1] = 'w';
        break;

    case Severity::error:
        buffer[1] = 'E';
        break;
    }

    const auto msg_size = strlen(msg);

    u32 i;
    constexpr size_t prefix_size = 3;
    for (i = 0; i < std::min(msg_size, buffer.size() - (prefix_size + 1));
         ++i) {
        buffer[i + 3] = msg[i];
    }
    buffer[i + 3] = '\n';

    flash_save(buffer, log_write_loc);

    log_write_loc += msg_size + prefix_size + 1;
}


////////////////////////////////////////////////////////////////////////////////
// Speaker
////////////////////////////////////////////////////////////////////////////////

#define REG_SND1SWEEP		*(volatile u16*)(0x04000000+0x0060)	//!< Channel 1 Sweep
#define REG_SND1CNT			*(volatile u16*)(0x04000000+0x0062)	//!< Channel 1 Control
#define REG_SND1FREQ		*(volatile u16*)(0x04000000+0x0064)	//!< Channel 1 frequency
#define REG_SND1FREQ *(volatile u16*)(0x04000000+0x0064)
#define REG_SNDCNT *(volatile u32*)(0x04000000+0x0080)	//!< Main sound control
#define REG_SNDDMGCNT *(volatile u16*)(0x04000000+0x0080)	//!< DMG channel control
#define REG_SNDDSCNT *(volatile u16*)(0x04000000+0x0082)	//!< Direct Sound control
#define REG_SNDSTAT *(volatile u16*)(0x04000000+0x0084)	//!< Sound status
#define REG_SNDBIAS *(volatile u16*)(0x04000000+0x0088)	//!< Sound bias
#define REG_SND2CNT *(volatile u16*)(0x04000000+0x0068)	//!< Channel 2 control
#define REG_SND2FREQ *(volatile u16*)(0x04000000+0x006C)	//!< Channel 2 frequency


// --- REG_SND1SWEEP ---------------------------------------------------

/*!	\defgroup grpAudioSSW	Tone Generator, Sweep Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SND1SWEEP (aka REG_SOUND1CNT_L)
*/
/*!	\{	*/

#define SSW_INC			 0		//!< Increasing sweep rate
#define SSW_DEC			0x0008	//!< Decreasing sweep rate
#define SSW_OFF         0x0008	//!< Disable sweep altogether

#define SSW_SHIFT_MASK	0x0007
#define SSW_SHIFT_SHIFT		 0
#define SSW_SHIFT(n)	((n)<<SSW_SHIFT_SHIFT)

#define SSW_TIME_MASK	0x0070
#define SSW_TIME_SHIFT		 4
#define SSW_TIME(n)		((n)<<SSW_TIME_SHIFT)


#define SSW_BUILD(shift, dir, time) \
	( (((time)&7)<<4) | ((dir)<<3) | ((shift)&7) )

/*!	\}	/defgroup	*/

// --- REG_SND1CNT, REG_SND2CNT, REG_SND4CNT ---------------------------

/*!	\defgroup grpAudioSSQR	Tone Generator, Square Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SND{1,2,4}CNT
	(aka REG_SOUND1CNT_H, REG_SOUND2CNT_L, REG_SOUND4CNT_L, respectively)
*/
/*!	\{	*/

#define SSQR_DUTY1_8		 0	//!< 12.5% duty cycle (#-------)
#define SSQR_DUTY1_4	0x0040	//!< 25% duty cycle (##------)
#define SSQR_DUTY1_2	0x0080	//!< 50% duty cycle (####----)
#define SSQR_DUTY3_4	0x00C0	//!< 75% duty cycle (######--) Equivalent to 25%
#define SSQR_INC			 0	//!< Increasing volume
#define SSQR_DEC		0x0800	//!< Decreasing volume

#define SSQR_LEN_MASK	0x003F
#define SSQR_LEN_SHIFT		 0
#define SSQR_LEN(n)		((n)<<SSQR_LEN_SHIFT)

#define SSQR_DUTY_MASK	0x00C0
#define SSQR_DUTY_SHIFT		 6
#define SSQR_DUTY(n)	((n)<<SSQR_DUTY_SHIFT)

#define SSQR_TIME_MASK	0x0700
#define SSQR_TIME_SHIFT		 8
#define SSQR_TIME(n)	((n)<<SSQR_TIME_SHIFT)

#define SSQR_IVOL_MASK	0xF000
#define SSQR_IVOL_SHIFT		12
#define SSQR_IVOL(n)	((n)<<SSQR_IVOL_SHIFT)


#define SSQR_ENV_BUILD(ivol, dir, time)				\
	(  ((ivol)<<12) | ((dir)<<11) | (((time)&7)<<8) )

#define SSQR_BUILD(_ivol, dir, step, duty, len)		\
	( SSQR_ENV_BUILD(ivol,dir,step) | (((duty)&3)<<6) | ((len)&63) )


/*!	\}	/defgroup	*/

// --- REG_SND1FREQ, REG_SND2FREQ, REG_SND3FREQ ------------------------

/*!	\defgroup grpAudioSFREQ	Tone Generator, Frequency Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SND{1-3}FREQ
	(aka REG_SOUND1CNT_X, REG_SOUND2CNT_H, REG_SOUND3CNT_X)
*/
/*!	\{	*/

#define SFREQ_HOLD				 0	//!< Continuous play
#define SFREQ_TIMED			0x4000	//!< Timed play
#define SFREQ_RESET			0x8000	//!< Reset sound

#define SFREQ_RATE_MASK		0x07FF
#define SFREQ_RATE_SHIFT		 0
#define SFREQ_RATE(n)		((n)<<SFREQ_RATE_SHIFT)

#define SFREQ_BUILD(rate, timed, reset)				\
	( ((rate)&0x7FF) | ((timed)<<14) | ((reset)<<15) )

/*!	\}	/defgroup	*/

// --- REG_SNDDMGCNT ---------------------------------------------------

/*!	\defgroup grpAudioSDMG	Tone Generator, Control Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SNDDMGCNT (aka REG_SOUNDCNT_L)
*/
/*!	\{	*/


#define SDMG_LSQR1		0x0100	//!< Enable channel 1 on left
#define SDMG_LSQR2		0x0200	//!< Enable channel 2 on left
#define SDMG_LWAVE		0x0400	//!< Enable channel 3 on left
#define SDMG_LNOISE		0x0800	//!< Enable channel 4 on left
#define SDMG_RSQR1		0x1000	//!< Enable channel 1 on right
#define SDMG_RSQR2		0x2000	//!< Enable channel 2 on right
#define SDMG_RWAVE		0x4000	//!< Enable channel 3 on right
#define SDMG_RNOISE		0x8000	//!< Enable channel 4 on right

#define SDMG_LVOL_MASK	0x0007
#define SDMG_LVOL_SHIFT		 0
#define SDMG_LVOL(n)	((n)<<SDMG_LVOL_SHIFT)

#define SDMG_RVOL_MASK	0x0070
#define SDMG_RVOL_SHIFT		 4
#define SDMG_RVOL(n)	((n)<<SDMG_RVOL_SHIFT)


// Unshifted values
#define SDMG_SQR1		0x01
#define SDMG_SQR2		0x02
#define SDMG_WAVE		0x04
#define SDMG_NOISE		0x08


#define SDMG_BUILD(_lmode, _rmode, _lvol, _rvol)	\
	( ((_rmode)<<12) | ((_lmode)<<8) | (((_rvol)&7)<<4) | ((_lvol)&7) )

#define SDMG_BUILD_LR(_mode, _vol) SDMG_BUILD(_mode, _mode, _vol, _vol)

/*!	\}	/defgroup	*/

// --- REG_SNDDSCNT ----------------------------------------------------

/*!	\defgroup grpAudioSDS	Direct Sound Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SNDDSCNT (aka REG_SOUNDCNT_H)
*/
/*!	\{	*/

#define SDS_DMG25			 0	//!< Tone generators at 25% volume
#define SDS_DMG50		0x0001	//!< Tone generators at 50% volume
#define SDS_DMG100		0x0002	//!< Tone generators at 100% volume
#define SDS_A50			 0	//!< Direct Sound A at 50% volume
#define SDS_A100		0x0004	//!< Direct Sound A at 100% volume
#define SDS_B50			 0	//!< Direct Sound B at 50% volume
#define SDS_B100		0x0008	//!< Direct Sound B at 100% volume
#define SDS_AR			0x0100	//!< Enable Direct Sound A on right
#define SDS_AL			0x0200	//!< Enable Direct Sound A on left
#define SDS_ATMR0			 0	//!< Direct Sound A to use timer 0
#define SDS_ATMR1		0x0400	//!< Direct Sound A to use timer 1
#define SDS_ARESET		0x0800	//!< Reset FIFO of Direct Sound A
#define SDS_BR			0x1000	//!< Enable Direct Sound B on right
#define SDS_BL			0x2000	//!< Enable Direct Sound B on left
#define SDS_BTMR0			 0	//!< Direct Sound B to use timer 0
#define SDS_BTMR1		0x4000	//!< Direct Sound B to use timer 1
#define SDS_BRESET		0x8000	//!< Reset FIFO of Direct Sound B

/*!	\}	/defgroup	*/

// --- REG_SNDSTAT -----------------------------------------------------

/*!	\defgroup grpAudioSSTAT	Sound Status Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SNDSTAT (and REG_SOUNDCNT_X)
*/
/*!	\{	*/

#define SSTAT_SQR1		0x0001	//!< (R) Channel 1 status
#define SSTAT_SQR2		0x0002	//!< (R) Channel 2 status
#define SSTAT_WAVE		0x0004	//!< (R) Channel 3 status
#define SSTAT_NOISE		0x0008	//!< (R) Channel 4 status
#define SSTAT_DISABLE		 0	//!< Disable sound
#define SSTAT_ENABLE	0x0080	//!< Enable sound. NOTE: enable before using any other sound regs


// Rates for traditional notes in octave +5
const u32 __snd_rates[12]=
{
    8013, 7566, 7144, 6742, // C , C#, D , D#
    6362, 6005, 5666, 5346, // E , F , F#, G
    5048, 4766, 4499, 4246  // G#, A , A#, B
};


#define SND_RATE(note, oct) ( 2048-(__snd_rates[note]>>((oct))) )


void Platform::Speaker::play(Note n, Octave o, Channel c)
{
    switch (c) {
    case 0:
        REG_SND1FREQ = SFREQ_RESET | SND_RATE(int(n), o);
        break;

    case 1:
        REG_SND2FREQ = SFREQ_RESET | SND_RATE(int(n), o);
        break;
    }
}

Platform::Speaker::Speaker()
{
    // turn sound on
    REG_SNDSTAT = SSTAT_ENABLE;
    // snd1 on left/right ; both full volume
    REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR1 | SDMG_SQR2, 7);
    // DMG ratio to 100%
    REG_SNDDSCNT = SDS_DMG100;

    // no sweep
    REG_SND1SWEEP = SSW_OFF;

    // envelope: vol=12, decay, max step time (7) ; 50% duty
    REG_SND1CNT = SSQR_ENV_BUILD(12, 0, 7) | SSQR_DUTY1_4;
    REG_SND1FREQ = 0;

    REG_SND2CNT = SSQR_ENV_BUILD(10, 0, 7) | SSQR_DUTY1_4;
    REG_SND2FREQ = 0;
}


#endif // __GBA__
