
////////////////////////////////////////////////////////////////////////////////
//
//
// Gameboy Advance Platform
//
//
////////////////////////////////////////////////////////////////////////////////

#ifdef __GBA__

#include "graphics/overlay.hpp"
#include "number/random.hpp"
#include "platform.hpp"
#include "string.hpp"
#include "util.hpp"
#include <algorithm>


// These word and halfword versions of memcpy are written in assembly. They use
// special ARM instructions to copy data faster than you could do with thumb
// code.
extern "C" {
__attribute__((section(".iwram"), long_call)) void
memcpy32(void* dst, const void* src, uint wcount);
void memcpy16(void* dst, const void* src, uint hwcount);
}


void start(Platform&);


static Platform* platform;


int main()
{
    Platform pf;
    ::platform = &pf;

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

#define SE_PALBANK_MASK 0xF000
#define SE_PALBANK_SHIFT 12
#define SE_PALBANK(n) ((n) << SE_PALBANK_SHIFT)

#define ATTR2_PALBANK_MASK 0xF000
#define ATTR2_PALBANK_SHIFT 12
#define ATTR2_PALBANK(n) ((n) << ATTR2_PALBANK_SHIFT)

#define ATTR2_PRIO_SHIFT 10
#define ATTR2_PRIO(n) ((n) << ATTR2_PRIO_SHIFT)
#define ATTR2_PRIORITY(n) ATTR2_PRIO(n)


////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


DeltaClock::DeltaClock() : impl_(nullptr)
{
}


static volatile u16* const scanline = (u16*)0x4000006;


constexpr Microseconds fixed_step = 16667;


Microseconds DeltaClock::reset()
{
    // (1 second / 60 frames) x (1,000,000 microseconds / 1 second) =
    // 16,666.6...
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
    std::copy(std::begin(states_), std::end(states_), std::begin(prev_));

    states_[int(Key::action_1)] = ~(*keys) & KEY_A;
    states_[int(Key::action_2)] = ~(*keys) & KEY_B;
    states_[int(Key::start)] = ~(*keys) & KEY_START;
    states_[int(Key::select)] = ~(*keys) & KEY_SELECT;
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

static ObjectAttributes
    object_attribute_back_buffer[Platform::Screen::sprite_limit];


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
#define BG_CBB_MASK 0x000C
#define BG_CBB_SHIFT 2
#define BG_CBB(n) ((n) << BG_CBB_SHIFT)

#define BG_SBB_MASK 0x1F00
#define BG_SBB_SHIFT 8
#define BG_SBB(n) ((n) << BG_SBB_SHIFT)

#define BG_SIZE_MASK 0xC000
#define BG_SIZE_SHIFT 14
#define BG_SIZE(n) ((n) << BG_SIZE_SHIFT)

#define BG_PRIORITY(m) ((m))

#define BG_REG_32x32 0      //!< reg bg, 32x32 (256x256 px)
#define BG_REG_64x32 0x4000 //!< reg bg, 64x32 (512x256 px)
#define BG_REG_32x64 0x8000 //!< reg bg, 32x64 (256x512 px)
#define BG_REG_64x64 0xC000 //!< reg bg, 64x64 (512x512 px)


static volatile u16* bg0_control = (volatile u16*)0x4000008;
static volatile u16* bg1_control = (volatile u16*)0x400000a;
static volatile u16* bg2_control = (volatile u16*)0x400000c;
// static volatile u16* bg3_control = (volatile u16*)0x400000e;


static volatile short* bg0_x_scroll = (volatile short*)0x4000010;
static volatile short* bg0_y_scroll = (volatile short*)0x4000012;
static volatile short* bg1_x_scroll = (volatile short*)0x4000014;
static volatile short* bg1_y_scroll = (volatile short*)0x4000016;
// static volatile short* bg2_x_scroll = (volatile short*)0x4000018;
// static volatile short* bg2_y_scroll = (volatile short*)0x400001a;
// static volatile short* bg3_x_scroll = (volatile short*)0x400001c;
// static volatile short* bg3_y_scroll = (volatile short*)0x400001e;


static u8 last_fade_amt;
static ColorConstant last_color;


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
    REG_DISPCNT =
        MODE_0 | OBJ_ENABLE | OBJ_MAP_1D | BG0_ENABLE | BG1_ENABLE | BG2_ENABLE;

    *reg_blendcnt = BLD_BUILD(BLD_OBJ, BLD_BG0 | BLD_BG1, 0);

    *reg_blendalpha = BLDA_BUILD(0x40 / 8, 0x40 / 8);

    *bg0_control = BG_SBB(8) | BG_REG_64x64 | BG_PRIORITY(3);
    //        0xC800; // 64x64, 0x0100 for 32x32
    *bg1_control = BG_SBB(12) | BG_PRIORITY(3);
    *bg2_control = BG_SBB(13) | BG_PRIORITY(0) | BG_CBB(3);

    view_.set_size(this->size().cast<Float>());
}


static u32 last_oam_write_index = 0;
static u32 oam_write_index = 0;


const Color& real_color(ColorConstant k)
{
    switch (k) {
    case ColorConstant::spanish_crimson:
        static const Color spn_crimson(29, 3, 11);
        return spn_crimson;

    case ColorConstant::electric_blue:
        static const Color el_blue(0, 31, 31);
        return el_blue;

    case ColorConstant::turquoise_blue:
        static const Color turquoise_blue(0, 31, 27);
        return turquoise_blue;

    case ColorConstant::picton_blue:
        static const Color picton_blue(9, 20, 31);
        return picton_blue;

    case ColorConstant::aerospace_orange:
        static const Color aerospace_orange(31, 10, 0);
        return aerospace_orange;

    case ColorConstant::safety_orange:
        static const Color safety_orange(31, 14, 0);
        return safety_orange;

    case ColorConstant::stil_de_grain:
        static const Color stil_de_grain(30, 27, 11);
        return stil_de_grain;

    case ColorConstant::steel_blue:
        static const Color steel_blue(6, 10, 16);
        return steel_blue;

    case ColorConstant::silver_white:
        static const Color silver_white(29, 29, 30);
        return silver_white;

    default:
    case ColorConstant::null:
    case ColorConstant::rich_black:
        static const Color rich_black(0, 0, 2);
        return rich_black;
    }
}


using PaletteBank = int;
constexpr PaletteBank available_palettes = 3;
constexpr PaletteBank palette_count = 16;

static PaletteBank palette_counter = available_palettes;


// For the purpose of saving cpu cycles. The color_mix function scans a list of
// previous results, and if one matches the current blend parameters, the caller
// will set the locked_ field to true, and return the index of the existing
// palette bank. Each call to display() unlocks all of the palette infos.
static struct PaletteInfo {
    ColorConstant color_ = ColorConstant::null;
    u8 blend_amount_ = 0;
    bool locked_ = false;
} palette_info[palette_count] = {};


// We want to be able to disable color mixes during a screen fade. We perform a
// screen fade by blending a color into the base palette. If we allow sprites to
// use other palette banks during a screen fade, they won't be faded, because
// they are not using the first palette bank.
static bool color_mix_disabled = false;


// Perform a color mix between the spritesheet palette bank (bank zero), and
// return the palette bank where the resulting mixture is stored. We can only
// display 12 mixed colors at a time, because the first four banks are in use.
static PaletteBank color_mix(ColorConstant k, u8 amount)
{
    if (UNLIKELY(color_mix_disabled)) {
        return 0;
    }

    for (PaletteBank palette = available_palettes; palette < 16; ++palette) {
        auto& info = palette_info[palette];
        if (info.color_ == k and info.blend_amount_ == amount) {
            info.locked_ = true;
            return palette;
        }
    }

    // Skip over any palettes that are in use
    while (palette_info[palette_counter].locked_) {
        if (UNLIKELY(palette_counter == palette_count)) {
            return 0;
        }
        ++palette_counter;
    }

    if (UNLIKELY(palette_counter == palette_count)) {
        return 0; // Exhausted all the palettes that we have for effects.
    }

    const auto& c = real_color(k);

    for (int i = 0; i < 16; ++i) {
        auto from = Color::from_bgr_hex_555(MEM_PALETTE[i]);
        const u32 index = 16 * palette_counter + i;
        MEM_PALETTE[index] = Color(fast_interpolate(c.r_, from.r_, amount),
                                   fast_interpolate(c.g_, from.g_, amount),
                                   fast_interpolate(c.b_, from.b_, amount))
                                 .bgr_hex_555();
    }

    palette_info[palette_counter] = {k, amount, true};

    return palette_counter++;
}


void Platform::Screen::draw(const Sprite& spr)
{
    if (UNLIKELY(spr.get_alpha() == Sprite::Alpha::transparent)) {
        return;
    }

    const auto pb = [&]() -> PaletteBank {
        const auto& mix = spr.get_mix();
        if (UNLIKELY(mix.color_ not_eq ColorConstant::null)) {
            if (const auto pal_bank = color_mix(mix.color_, mix.amount_)) {
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
        auto oa = object_attribute_back_buffer + oam_write_index;
        if (spr.get_alpha() not_eq Sprite::Alpha::translucent) {
            oa->attribute_0 = ATTR0_COLOR_16 | ATTR0_TALL;
        } else {
            oa->attribute_0 = ATTR0_COLOR_16 | ATTR0_TALL | ATTR0_BLEND;
        }
        oa->attribute_1 = ATTR1_SIZE_32;

        const auto& flip = spr.get_flip();
        oa->attribute_1 |= ((int)flip.y << 13);
        oa->attribute_1 |= ((int)flip.x << 12);

        const auto abs_position = position - view_center;
        oa->attribute_0 &= 0xff00;
        oa->attribute_0 |= abs_position.y & 0x00ff;
        oa->attribute_1 &= 0xfe00;
        oa->attribute_1 |= (abs_position.x + x_off) & 0x01ff;
        oa->attribute_2 = 2 + spr.get_texture_index() * scale + tex_off;
        oa->attribute_2 |= pb;
        oa->attribute_2 |= ATTR2_PRIORITY(2);
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

        // When a sprite is flipped, each of the individual 32x16 strips are
        // flipped, and then we need to swap the drawing X-offsets, so that the
        // second strip will be drawn first.
        if (not spr.get_flip().x) {
            draw_sprite(0, 0, 16);
            draw_sprite(8, 16, 16);

        } else {
            draw_sprite(0, 16, 16);
            draw_sprite(8, 0, 16);
        }

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

    // static int index;
    // constexpr int sample_count = 32;
    // static int buffer[32];
    // static std::optional<Text> text;

    // auto tm = platform->stopwatch().stop();

    // if (index < sample_count) {
    //     buffer[index++] = tm;

    // } else {
    //     index = 0;

    //     int accum = 0;
    //     for (int i = 0; i < sample_count; ++i) {
    //         accum += buffer[i];
    //     }

    //     accum /= 32;

    //     text.emplace(*platform, OverlayCoord{1, 1});
    //     text->assign(((accum * 59.59f) / 16666666.66) * 100);
    //     text->append(" percent");
    // }

    // VSync
    VBlankIntrWait();
}


void Platform::Screen::display()
{
    // platform->stopwatch().start();

    for (u32 i = oam_write_index; i < last_oam_write_index; ++i) {
        object_attribute_back_buffer[i].attribute_0 |= attr0_mask::disabled;
    }

    // I noticed less graphical artifacts when using a back buffer. I thought I
    // would see better performance when writing directly to OAM, rather than
    // doing a copy later, but I did not notice any performance difference when
    // adding a back buffer.
    memcpy32(object_attribute_memory,
             object_attribute_back_buffer,
             (sizeof object_attribute_back_buffer) / 4);

    last_oam_write_index = oam_write_index;
    oam_write_index = 0;
    palette_counter = available_palettes;

    for (auto& info : palette_info) {
        info.locked_ = false;
    }

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


#include "graphics/images.h"


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


static const TextureData sprite_textures[] = {TEXTURE_INFO(spritesheet),
                                              TEXTURE_INFO(spritesheet2),
                                              TEXTURE_INFO(spritesheet_boss0),
                                              TEXTURE_INFO(spritesheet_boss1)};


static const TextureData tile_textures[] = {TEXTURE_INFO(tilesheet),
                                            TEXTURE_INFO(tilesheet2)};


static const TextureData overlay_textures[] = {
    TEXTURE_INFO(overlay),
    TEXTURE_INFO(overlay_journal),
    TEXTURE_INFO(old_poster_flattened)};


static const TextureData* current_spritesheet = &sprite_textures[0];
static const TextureData* current_tilesheet = &tile_textures[0];


static bool validate_texture_size(Platform& pfrm, size_t size)
{
    constexpr auto charblock_size = sizeof(ScreenBlock) * 8;
    if (size > charblock_size) {
        error(pfrm, "tileset exceeds charblock size");
        return false;
    }
    return true;
}


void Platform::load_overlay_texture(const char* name)
{
    for (auto& info : overlay_textures) {

        if (strcmp(name, info.name_) == 0) {

            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(info.palette_data_[i]);
                MEM_BG_PALETTE[16 + i] = from.bgr_hex_555();
            }

            if (validate_texture_size(*this, info.tile_data_length_)) {
                // NOTE: this is the last charblock
                memcpy16((void*)&MEM_SCREENBLOCKS[24][0],
                         info.tile_data_,
                         info.tile_data_length_ / 2);
            }
        }
    }
}


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


void Platform::fatal()
{
    SoftReset(ROM_RESTART);
}


void Platform::set_overlay_tile(u16 x, u16 y, u16 val)
{
    if (x > 31 or y > 31) {
        return;
    }

    MEM_SCREENBLOCKS[13][x + y * 32] = val | SE_PALBANK(1);
}


void Platform::fill_overlay(u16 tile)
{
    for (unsigned i = 0; i < sizeof(ScreenBlock) / sizeof(u16); ++i) {
        MEM_SCREENBLOCKS[13][i] = tile | SE_PALBANK(1);
    }
}


u16 Platform::get_overlay_tile(u16 x, u16 y)
{
    if (x > 31 or y > 31) {
        return 0;
    }

    return MEM_SCREENBLOCKS[13][x + y * 32] & ~(SE_PALBANK(1));
}


static auto blend(const Color& c1, const Color& c2, u8 amt)
{
    switch (amt) {
    case 0:
        return c1.bgr_hex_555();
    case 255:
        return c2.bgr_hex_555();
    default:
        return Color(fast_interpolate(c2.r_, c1.r_, amt),
                     fast_interpolate(c2.g_, c1.g_, amt),
                     fast_interpolate(c2.b_, c1.b_, amt))
            .bgr_hex_555();
    }
}


void Platform::Screen::fade(float amount,
                            ColorConstant k,
                            std::optional<ColorConstant> base)
{
    const u8 amt = amount * 255;

    if (amt == 0) {
        color_mix_disabled = false;
    } else {
        color_mix_disabled = true;
    }

    if (amt == last_fade_amt and k == last_color) {
        return;
    }

    last_fade_amt = amt;
    last_color = k;

    const auto& c = real_color(k);

    if (not base) {
        for (int i = 0; i < 16; ++i) {
            auto from =
                Color::from_bgr_hex_555(current_spritesheet->palette_data_[i]);
            MEM_PALETTE[i] = blend(from, c, amt);
        }
        for (int i = 0; i < 16; ++i) {
            auto from =
                Color::from_bgr_hex_555(current_tilesheet->palette_data_[i]);
            MEM_BG_PALETTE[i] = blend(from, c, amt);
        }
        // Should the overlay really be part of the fade...? Tricky, sometimes
        // one wants do display some text over a faded out screen, so let's
        // leave it disabled for now.
        //
        // for (int i = 0; i < 16; ++i) {
        //     auto from = Color::from_bgr_hex_555(bgr_overlayPal[i]);
        //     MEM_BG_PALETTE[16 + i] = blend(from);
        // }
    } else {
        for (int i = 0; i < 16; ++i) {
            MEM_PALETTE[i] = blend(real_color(*base), c, amt);
            MEM_BG_PALETTE[i] = blend(real_color(*base), c, amt);
            // MEM_BG_PALETTE[16 + i] = blend(real_color(*base));
        }
    }
}


void Platform::load_sprite_texture(const char* name)
{
    for (auto& info : sprite_textures) {

        if (strcmp(name, info.name_) == 0) {

            current_spritesheet = &info;

            // NOTE: There are four tile blocks, so index four points to the
            // end of the tile memory.
            memcpy16((void*)&MEM_TILE[4][1],
                     info.tile_data_,
                     info.tile_data_length_ / 2);

            // We need to do this, otherwise whatever screen fade is currently
            // active will be overwritten by the copy.
            const auto& c = real_color(last_color);
            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(
                    current_spritesheet->palette_data_[i]);
                MEM_PALETTE[i] = blend(from, c, last_fade_amt);
            }
        }
    }
}


void Platform::load_tile_texture(const char* name)
{
    for (auto& info : tile_textures) {

        if (strcmp(name, info.name_) == 0) {

            current_tilesheet = &info;

            // We don't want to load the whole palette into memory, we might
            // overwrite palettes used by someone else, e.g. the overlay...
            //
            // Also, like the sprite texture, we need to apply the currently
            // active screen fade while modifying the color palette.
            const auto& c = real_color(last_color);
            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(
                    current_tilesheet->palette_data_[i]);
                MEM_BG_PALETTE[i] = blend(from, c, last_fade_amt);
            }

            if (validate_texture_size(*this, info.tile_data_length_)) {
                memcpy16((void*)&MEM_SCREENBLOCKS[0][0],
                         info.tile_data_,
                         info.tile_data_length_ / 2);
            }
        }
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

bool Platform::write_save(const PersistentData& data)
{
    if (not flash_save(data, 0)) {
        return false;
    }

    // Sanity check, that writing the save file succeeded.
    const auto d = flash_load<PersistentData>(0);
    if (d.magic_ not_eq PersistentData::magic_val) {
        return false;
    }
    return true;
}


std::optional<PersistentData> Platform::read_save()
{
    auto sd = flash_load<PersistentData>(0);
    if (sd.magic_ == PersistentData::magic_val) {
        return sd;
    }
    return {};
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


// NOTE: PersistentData goes first into flash memory, followed by the game's
// logs.
static u32 log_write_loc = sizeof(PersistentData);


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

    const auto msg_size = str_len(msg);

    u32 i;
    constexpr size_t prefix_size = 3;
    for (i = 0;
         i < std::min(size_t(msg_size), buffer.size() - (prefix_size + 1));
         ++i) {
        buffer[i + 3] = msg[i];
    }
    buffer[i + 3] = '\n';

    flash_save(buffer, log_write_loc);

    log_write_loc += msg_size + prefix_size + 1;
}


////////////////////////////////////////////////////////////////////////////////
// Speaker
//
// For music, the Speaker class uses the GameBoy's direct sound chip to play
// 8-bit signed raw audio, at 16kHz. For everything else, the game uses the
// waveform generators.
//
////////////////////////////////////////////////////////////////////////////////

//! \name Channel 1: Square wave with sweep
//\{
#define REG_SND1SWEEP *(vu16*)(0x04000000 + 0x0060) //!< Channel 1 Sweep
#define REG_SND1CNT *(vu16*)(0x04000000 + 0x0062)   //!< Channel 1 Control
#define REG_SND1FREQ *(vu16*)(0x04000000 + 0x0064)  //!< Channel 1 frequency
//\}

//! \name Channel 2: Simple square wave
//\{
#define REG_SND2CNT *(vu16*)(0x04000000 + 0x0068)  //!< Channel 2 control
#define REG_SND2FREQ *(vu16*)(0x04000000 + 0x006C) //!< Channel 2 frequency
//\}

//! \name Channel 3: Wave player
//\{
#define REG_SND3SEL *(vu16*)(0x04000000 + 0x0070)  //!< Channel 3 wave select
#define REG_SND3CNT *(vu16*)(0x04000000 + 0x0072)  //!< Channel 3 control
#define REG_SND3FREQ *(vu16*)(0x04000000 + 0x0074) //!< Channel 3 frequency
//\}

//! \name Channel 4: Noise generator
//\{
#define REG_SND4CNT *(vu16*)(0x04000000 + 0x0078)  //!< Channel 4 control
#define REG_SND4FREQ *(vu16*)(0x04000000 + 0x007C) //!< Channel 4 frequency
//\}

#define REG_SNDCNT *(volatile u32*)(0x04000000 + 0x0080) //!< Main sound control
#define REG_SNDDMGCNT                                                          \
    *(volatile u16*)(0x04000000 + 0x0080) //!< DMG channel control
#define REG_SNDDSCNT                                                           \
    *(volatile u16*)(0x04000000 + 0x0082) //!< Direct Sound control
#define REG_SNDSTAT *(volatile u16*)(0x04000000 + 0x0084) //!< Sound status
#define REG_SNDBIAS *(volatile u16*)(0x04000000 + 0x0088) //!< Sound bias


// --- REG_SND1SWEEP ---------------------------------------------------

/*!	\defgroup grpAudioSSW	Tone Generator, Sweep Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SND1SWEEP (aka REG_SOUND1CNT_L)
*/
/*!	\{	*/

#define SSW_INC 0      //!< Increasing sweep rate
#define SSW_DEC 0x0008 //!< Decreasing sweep rate
#define SSW_OFF 0x0008 //!< Disable sweep altogether

#define SSW_SHIFT_MASK 0x0007
#define SSW_SHIFT_SHIFT 0
#define SSW_SHIFT(n) ((n) << SSW_SHIFT_SHIFT)

#define SSW_TIME_MASK 0x0070
#define SSW_TIME_SHIFT 4
#define SSW_TIME(n) ((n) << SSW_TIME_SHIFT)


#define SSW_BUILD(shift, dir, time)                                            \
    ((((time)&7) << 4) | ((dir) << 3) | ((shift)&7))

/*!	\}	/defgroup	*/

// --- REG_SND1CNT, REG_SND2CNT, REG_SND4CNT ---------------------------

/*!	\defgroup grpAudioSSQR	Tone Generator, Square Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SND{1,2,4}CNT
	(aka REG_SOUND1CNT_H, REG_SOUND2CNT_L, REG_SOUND4CNT_L, respectively)
*/
/*!	\{	*/

#define SSQR_DUTY1_8 0      //!< 12.5% duty cycle (#-------)
#define SSQR_DUTY1_4 0x0040 //!< 25% duty cycle (##------)
#define SSQR_DUTY1_2 0x0080 //!< 50% duty cycle (####----)
#define SSQR_DUTY3_4 0x00C0 //!< 75% duty cycle (######--) Equivalent to 25%
#define SSQR_INC 0          //!< Increasing volume
#define SSQR_DEC 0x0800     //!< Decreasing volume

#define SSQR_LEN_MASK 0x003F
#define SSQR_LEN_SHIFT 0
#define SSQR_LEN(n) ((n) << SSQR_LEN_SHIFT)

#define SSQR_DUTY_MASK 0x00C0
#define SSQR_DUTY_SHIFT 6
#define SSQR_DUTY(n) ((n) << SSQR_DUTY_SHIFT)

#define SSQR_TIME_MASK 0x0700
#define SSQR_TIME_SHIFT 8
#define SSQR_TIME(n) ((n) << SSQR_TIME_SHIFT)

#define SSQR_IVOL_MASK 0xF000
#define SSQR_IVOL_SHIFT 12
#define SSQR_IVOL(n) ((n) << SSQR_IVOL_SHIFT)


#define SSQR_ENV_BUILD(ivol, dir, time)                                        \
    (((ivol) << 12) | ((dir) << 11) | (((time)&7) << 8))

#define SSQR_BUILD(_ivol, dir, step, duty, len)                                \
    (SSQR_ENV_BUILD(ivol, dir, step) | (((duty)&3) << 6) | ((len)&63))


/*!	\}	/defgroup	*/

// --- REG_SND1FREQ, REG_SND2FREQ, REG_SND3FREQ ------------------------

/*!	\defgroup grpAudioSFREQ	Tone Generator, Frequency Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SND{1-3}FREQ
	(aka REG_SOUND1CNT_X, REG_SOUND2CNT_H, REG_SOUND3CNT_X)
*/
/*!	\{	*/

#define SFREQ_HOLD 0       //!< Continuous play
#define SFREQ_TIMED 0x4000 //!< Timed play
#define SFREQ_RESET 0x8000 //!< Reset sound

#define SFREQ_RATE_MASK 0x07FF
#define SFREQ_RATE_SHIFT 0
#define SFREQ_RATE(n) ((n) << SFREQ_RATE_SHIFT)

#define SFREQ_BUILD(rate, timed, reset)                                        \
    (((rate)&0x7FF) | ((timed) << 14) | ((reset) << 15))

/*!	\}	/defgroup	*/

// --- REG_SNDDMGCNT ---------------------------------------------------

/*!	\defgroup grpAudioSDMG	Tone Generator, Control Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SNDDMGCNT (aka REG_SOUNDCNT_L)
*/
/*!	\{	*/


#define SDMG_LSQR1 0x0100  //!< Enable channel 1 on left
#define SDMG_LSQR2 0x0200  //!< Enable channel 2 on left
#define SDMG_LWAVE 0x0400  //!< Enable channel 3 on left
#define SDMG_LNOISE 0x0800 //!< Enable channel 4 on left
#define SDMG_RSQR1 0x1000  //!< Enable channel 1 on right
#define SDMG_RSQR2 0x2000  //!< Enable channel 2 on right
#define SDMG_RWAVE 0x4000  //!< Enable channel 3 on right
#define SDMG_RNOISE 0x8000 //!< Enable channel 4 on right

#define SDMG_LVOL_MASK 0x0007
#define SDMG_LVOL_SHIFT 0
#define SDMG_LVOL(n) ((n) << SDMG_LVOL_SHIFT)

#define SDMG_RVOL_MASK 0x0070
#define SDMG_RVOL_SHIFT 4
#define SDMG_RVOL(n) ((n) << SDMG_RVOL_SHIFT)


// Unshifted values
#define SDMG_SQR1 0x01
#define SDMG_SQR2 0x02
#define SDMG_WAVE 0x04
#define SDMG_NOISE 0x08


#define SDMG_BUILD(_lmode, _rmode, _lvol, _rvol)                               \
    (((_rmode) << 12) | ((_lmode) << 8) | (((_rvol)&7) << 4) | ((_lvol)&7))

#define SDMG_BUILD_LR(_mode, _vol) SDMG_BUILD(_mode, _mode, _vol, _vol)

/*!	\}	/defgroup	*/

// --- REG_SNDDSCNT ----------------------------------------------------

/*!	\defgroup grpAudioSDS	Direct Sound Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SNDDSCNT (aka REG_SOUNDCNT_H)
*/
/*!	\{	*/

#define SDS_DMG25 0       //!< Tone generators at 25% volume
#define SDS_DMG50 0x0001  //!< Tone generators at 50% volume
#define SDS_DMG100 0x0002 //!< Tone generators at 100% volume
#define SDS_A50 0         //!< Direct Sound A at 50% volume
#define SDS_A100 0x0004   //!< Direct Sound A at 100% volume
#define SDS_B50 0         //!< Direct Sound B at 50% volume
#define SDS_B100 0x0008   //!< Direct Sound B at 100% volume
#define SDS_AR 0x0100     //!< Enable Direct Sound A on right
#define SDS_AL 0x0200     //!< Enable Direct Sound A on left
#define SDS_ATMR0 0       //!< Direct Sound A to use timer 0
#define SDS_ATMR1 0x0400  //!< Direct Sound A to use timer 1
#define SDS_ARESET 0x0800 //!< Reset FIFO of Direct Sound A
#define SDS_BR 0x1000     //!< Enable Direct Sound B on right
#define SDS_BL 0x2000     //!< Enable Direct Sound B on left
#define SDS_BTMR0 0       //!< Direct Sound B to use timer 0
#define SDS_BTMR1 0x4000  //!< Direct Sound B to use timer 1
#define SDS_BRESET 0x8000 //!< Reset FIFO of Direct Sound B

/*!	\}	/defgroup	*/

// --- REG_SNDSTAT -----------------------------------------------------

/*!	\defgroup grpAudioSSTAT	Sound Status Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SNDSTAT (and REG_SOUNDCNT_X)
*/
/*!	\{	*/

#define SSTAT_SQR1 0x0001  //!< (R) Channel 1 status
#define SSTAT_SQR2 0x0002  //!< (R) Channel 2 status
#define SSTAT_WAVE 0x0004  //!< (R) Channel 3 status
#define SSTAT_NOISE 0x0008 //!< (R) Channel 4 status
#define SSTAT_DISABLE 0    //!< Disable sound
#define SSTAT_ENABLE                                                           \
    0x0080 //!< Enable sound. NOTE: enable before using any other sound regs


// Rates for traditional notes in octave +5
const u32 __snd_rates[12] = {
    8013,
    7566,
    7144,
    6742, // C , C#, D , D#
    6362,
    6005,
    5666,
    5346, // E , F , F#, G
    5048,
    4766,
    4499,
    4246 // G#, A , A#, B
};


#define SND_RATE(note, oct) (2048 - (__snd_rates[note] >> ((oct))))


void Platform::Speaker::play_note(Note n, Octave o, Channel c)
{
    switch (c) {
    case 0:
        REG_SND1FREQ = SFREQ_RESET | SND_RATE(int(n), o);
        break;

    case 1:
        REG_SND2FREQ = SFREQ_RESET | SND_RATE(int(n), o);
        break;

    case 2:
        REG_SND3FREQ = SFREQ_RESET | SND_RATE(int(n), o);
        break;

    case 3:
        REG_SND4FREQ = SFREQ_RESET | SND_RATE(int(n), o);
        break;
    }
}

#define REG_SOUNDCNT_L                                                         \
    *(volatile u16*)0x4000080 //DMG sound control// #include "gba.h"
#define REG_SOUNDCNT_H *(volatile u16*)0x4000082 //Direct sound control
#define REG_SOUNDCNT_X *(volatile u16*)0x4000084 //Extended sound control
#define REG_DMA1SAD *(u32*)0x40000BC             //DMA1 Source Address
#define REG_DMA1DAD *(u32*)0x40000C0             //DMA1 Desination Address
#define REG_DMA1CNT_H *(u16*)0x40000C6           //DMA1 Control High Value
#define REG_TM1CNT_L *(u16*)0x4000104            //Timer 2 count value
#define REG_TM1CNT_H *(u16*)0x4000106            //Timer 2 control
#define REG_TM0CNT_L *(u16*)0x4000100            //Timer 0 count value
#define REG_TM0CNT_H *(u16*)0x4000102            //Timer 0 Control


#include "frostellar.hpp"
#include "october.hpp"
#include "sb_omega.hpp"


#define DEF_AUDIO(__STR_NAME__, __TRACK_NAME__)                                \
    {                                                                          \
        STR(__STR_NAME__), (AudioSample*)__TRACK_NAME__, __TRACK_NAME__##Len   \
    }


#include "gba_platform_soundcontext.hpp"


SoundContext snd_ctx;


static const struct AudioTrack {
    const char* name_;
    const AudioSample* data_;
    int length_;
} music_tracks[] = {DEF_AUDIO(ambience, frostellar),
                    DEF_AUDIO(sb_omega, sb_omega),
                    DEF_AUDIO(october, October)};


static const AudioTrack* find_track(const char* name)
{
    for (auto& track : music_tracks) {

        if (strcmp(name, track.name_) == 0) {
            return &track;
        }
    }

    return nullptr;
}


// NOTE: Between remixing the audio track down to 8-bit 16kHz signed, generating
// assembly output, adding the file to CMake, adding the include, and adding the
// sound to the sounds array, it's just too tedious to keep working this way...
#include "sound_blaster.hpp"
#include "sound_creak.hpp"
#include "sound_footstep1.hpp"
#include "sound_footstep2.hpp"
#include "sound_footstep3.hpp"
#include "sound_footstep4.hpp"
#include "sound_open_book.hpp"
#include "sound_openbag.hpp"
#include "sound_coin.hpp"

static const AudioTrack sounds[] = {DEF_AUDIO(footstep1, sound_footstep1),
                                    DEF_AUDIO(footstep2, sound_footstep2),
                                    DEF_AUDIO(footstep3, sound_footstep3),
                                    DEF_AUDIO(footstep4, sound_footstep4),
                                    DEF_AUDIO(open_book, sound_open_book),
                                    DEF_AUDIO(openbag, sound_openbag),
                                    DEF_AUDIO(blaster, sound_blaster),
                                    DEF_AUDIO(creak, sound_creak),
                                    DEF_AUDIO(coin, sound_coin)};


static const AudioTrack* get_sound(const char* name)
{
    for (auto& sound : sounds) {
        if (strcmp(name, sound.name_) == 0) {
            return &sound;
        }
    }
    return nullptr;
}


static std::optional<ActiveSoundInfo> make_sound(const char* name)
{
    if (auto sound = get_sound(name)) {
        return ActiveSoundInfo{0, sound->length_, sound->data_, 0};
    } else {
        return {};
    }
}


// If you're going to edit any of the variables used by the interrupt handler
// for audio playback, you should use this helper function.
template <typename F> auto modify_audio(F&& callback)
{
    irqDisable(IRQ_TIMER0);
    callback();
    irqEnable(IRQ_TIMER0);
}


bool Platform::Speaker::is_sound_playing(const char* name)
{
    if (auto sound = get_sound(name)) {
        bool playing = false;
        modify_audio([&] {
            for (const auto& info : snd_ctx.active_sounds) {
                if ((s8*)sound->data_ == info.data_) {
                    playing = true;
                    return;
                }
            }
        });

        return playing;
    }
    return false;
}


void Platform::Speaker::play_sound(const char* name, int priority)
{
    if (auto info = make_sound(name)) {
        info->priority_ = priority;

        modify_audio([&] {
            if (not snd_ctx.active_sounds.full()) {
                snd_ctx.active_sounds.push_back(*info);

            } else {
                ActiveSoundInfo* lowest = snd_ctx.active_sounds.begin();
                for (auto it = snd_ctx.active_sounds.begin();
                     it not_eq snd_ctx.active_sounds.end();
                     ++it) {
                    if (it->priority_ < lowest->priority_) {
                        lowest = it;
                    }
                }

                if (lowest not_eq snd_ctx.active_sounds.end() and
                    lowest->priority_ < priority) {
                    snd_ctx.active_sounds.erase(lowest);
                    snd_ctx.active_sounds.push_back(*info);
                }
            }
        });
    }
}


static void stop_music()
{
    modify_audio([] { snd_ctx.music_track = nullptr; });
}


void Platform::Speaker::stop_music()
{
    ::stop_music();
}


static void play_music(const char* name, bool loop)
{
    const auto track = find_track(name);
    if (track == nullptr) {
        return;
    }

    modify_audio([&] {
        snd_ctx.music_track_length = track->length_;
        snd_ctx.music_track_loop = loop;
        snd_ctx.music_track = track->data_;
        snd_ctx.music_track_pos = 0;
    });
}


void Platform::Speaker::load_music(const char* name, bool loop)
{
    // NOTE: The sound sample needs to be mono, and 8-bit signed. To export this
    // format from Audacity, convert the tracks to mono via the Tracks dropdown,
    // and then export as raw, in the format 8-bit signed.
    //
    // Also, important to convert the sound file frequency to 16kHz.

    this->stop_music();

    ::play_music(name, loop);
}


Platform::Speaker::Speaker()
{
}


////////////////////////////////////////////////////////////////////////////////
// Stopwatch
////////////////////////////////////////////////////////////////////////////////


#define REG_TM2CNT *(vu32*)(0x04000000 + 0x108)
#define REG_TM2CNT_L *(vu16*)(REG_BASE + 0x108)
#define REG_TM2CNT_H *(vu16*)(REG_BASE + 0x10a)


struct StopwatchData {
};


Platform::Stopwatch::Stopwatch()
{
    // ... TODO ...
}

static size_t stopwatch_total;

void Platform::Stopwatch::start()
{
    // auto sd = reinterpret_cast<StopwatchData*>(impl_);
    stopwatch_total = 0;

    // FIXME: use TM3, TM2 is now in use
    // REG_TM2CNT_L = 0;
    // REG_TM2CNT_H = 1 << 7 | 1 << 6;
}


int Platform::Stopwatch::stop()
{
    // auto sd = reinterpret_cast<StopwatchData*>(impl_);
    // FIXME: use TM3, TM2 is now in use
    // auto ret = REG_TM2CNT_L;
    // REG_TM2CNT_H = 0;

    return 0; // ret + stopwatch_total;
}


////////////////////////////////////////////////////////////////////////////////
// Misc
////////////////////////////////////////////////////////////////////////////////


s32 fast_divide(s32 numerator, s32 denominator)
{
    // NOTE: This is a BIOS call, but still faster than real division.
    return Div(numerator, denominator);
}


s32 fast_mod(s32 numerator, s32 denominator)
{
    return DivMod(numerator, denominator);
}


#define REG_SGFIFOA *(volatile u32*)0x40000A0


static void audio_update()
{
    alignas(4) AudioSample mixing_buffer[4];

    if (snd_ctx.music_track) {

        for (int i = 0; i < 4; ++i) {
            mixing_buffer[i] = snd_ctx.music_track[snd_ctx.music_track_pos++];
        }

        if (snd_ctx.music_track_pos > snd_ctx.music_track_length) {
            if (snd_ctx.music_track_loop) {
                snd_ctx.music_track_pos = 0;

            } else {
                snd_ctx.music_track = nullptr;
            }
        }

    } else {

        for (auto& val : mixing_buffer) {
            val = 0;
        }
    }

    // Maybe the world's worst sound mixing code...
    for (auto it = snd_ctx.active_sounds.begin();
         it not_eq snd_ctx.active_sounds.end();) {
        if (UNLIKELY(it->position_ + 4 >= it->length_)) {
            it = snd_ctx.active_sounds.erase(it);

        } else {
            for (int i = 0; i < 4; ++i) {
                mixing_buffer[i] += it->data_[it->position_++];
            }

            ++it;
        }
    }

    REG_SGFIFOA = *((u32*)mixing_buffer);
}


Platform::Platform()
{
    irqInit();
    irqEnable(IRQ_VBLANK);

    // irqEnable(IRQ_TIMER2);
    // irqSet(IRQ_TIMER2, [] {
    //     stopwatch_total += 0xffff;

    // FIXME: use TM3, TM2 is now in use
    //     REG_TM2CNT_H = 0;
    //     REG_TM2CNT_L = 0;
    //     REG_TM2CNT_H = 1 << 7 | 1 << 6;
    // });

    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 32; ++j) {
            set_overlay_tile(i, j, 0);
        }
    }

    REG_SOUNDCNT_H =
        0x0B0F; //DirectSound A + fifo reset + max volume to L and R
    REG_SOUNDCNT_X = 0x0080; //turn sound chip on

    irqEnable(IRQ_TIMER1);
    irqSet(IRQ_TIMER1, audio_update);

    REG_TM0CNT_L = 0xffff;
    REG_TM1CNT_L = 0xffff - 3; // I think that this is correct, but I'm not
                               // certain... so we want to play four samples at
                               // a time, which means that by subtracting three
                               // from the initial count, the timer will
                               // overflow at the correct rate, right?

    REG_TM0CNT_H = 0x00C3;
    REG_TM1CNT_H = 0x00C3;
}


#endif // __GBA__
