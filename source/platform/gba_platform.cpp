
////////////////////////////////////////////////////////////////////////////////
//
//
// Gameboy Advance Platform
//
//
////////////////////////////////////////////////////////////////////////////////

#ifdef __GBA__

#include "bulkAllocator.hpp"
#include "graphics/overlay.hpp"
#include "number/random.hpp"
#include "platform.hpp"
#include "string.hpp"
#include "util.hpp"
#include <algorithm>


void english__to_string(int num, char* buffer, int base);


#include "gba.h"


static int overlay_y = 0;


Platform::DeviceName Platform::device_name() const
{
    return "GameboyAdvance";
}


// These word and halfword versions of memcpy are written in assembly. They use
// special ARM instructions to copy data faster than you could do with thumb
// code.
extern "C" {
__attribute__((section(".iwram"), long_call)) void
memcpy32(void* dst, const void* src, uint wcount);
void memcpy16(void* dst, const void* src, uint hwcount);
}


////////////////////////////////////////////////////////////////////////////////
//
// Tile Memory Layout:
//
// The game uses every single available screen block, so the data is fairly
// tightly packed. Here's a chart representing the layout:
//
// All units of length are in screen blocks, followed by the screen block
// indices in parentheses. The texture data needs to be aligned to char block
// boundaries (eight screen blocks in a char block), which is why there is
// tilemap data packed into the screen blocks between sets of texture data.
//
//        charblock 0                      charblock 1
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// o============================================================
// |  t0 texture   | overlay mem |   t1 texture   |   bg mem   |
// | len 7 (0 - 6) |  len 1 (7)  | len 7 (8 - 14) | len 1 (15) | ...
// o============================================================
//
//        charblock 2                 charblock 3
//     ~~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//     ======================================================o
//     | overlay texture |     t0 mem      |     t1 mem      |
// ... | len 8 (16 - 23) | len 4 (24 - 27) | len 4 (28 - 31) |
//     ======================================================o
//

static constexpr const int sbb_per_cbb = 8; // ScreenBaseBlock per CharBaseBlock

static constexpr const int sbb_overlay_tiles = 7;
static constexpr const int sbb_bg_tiles = 15;
static constexpr const int sbb_t0_tiles = 24;
static constexpr const int sbb_t1_tiles = 28;

static constexpr const int sbb_overlay_texture = 16;
static constexpr const int sbb_t0_texture = 0;
static constexpr const int sbb_t1_texture = 8;
static constexpr const int sbb_bg_texture = sbb_t0_texture;

static constexpr const int cbb_overlay_texture =
    sbb_overlay_texture / sbb_per_cbb;

static constexpr const int cbb_t0_texture = sbb_t0_texture / sbb_per_cbb;
static constexpr const int cbb_t1_texture = sbb_t1_texture / sbb_per_cbb;
static constexpr const int cbb_bg_texture = sbb_bg_texture / sbb_per_cbb;


//
//
////////////////////////////////////////////////////////////////////////////////


void start(Platform&);


static Platform* platform;


// Thanks to Windows, main is technically platform specific (WinMain)
int main()
{
    Platform pf;
    ::platform = &pf;

    start(pf);
}


////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


Platform::DeltaClock::DeltaClock() : impl_(nullptr)
{
}


static size_t delta_total;


static int delta_read_tics()
{
    return REG_TM3CNT_L + delta_total;
}


static Microseconds delta_convert_tics(int tics)
{
    //
    // IMPORTANT: Already well into development, I discovered that the Gameboy
    // Advance does not refresh at exactly 60 frames per second. Rather than
    // change all of the code, I am going to keep the timestep as-is. Anyone
    // porting the code to a new platform should make the appropriate
    // adjustments in their implementation of DeltaClock. I believe the actual
    // refresh rate on the GBA is something like 59.59.
    //
    return ((tics * (59.59f / 60.f)) * 60.f) / 1000.f;
}


Platform::DeltaClock::TimePoint Platform::DeltaClock::sample() const
{
    return delta_read_tics();
}


Microseconds Platform::DeltaClock::duration(TimePoint t1, TimePoint t2)
{
    return delta_convert_tics(t2 - t1);
}


Microseconds Platform::DeltaClock::reset()
{
    // (1 second / 60 frames) x (1,000,000 microseconds / 1 second) =
    // 16,666.6...

    irqDisable(IRQ_TIMER3);
    const auto tics = delta_read_tics();
    REG_TM3CNT_H = 0;

    irqEnable(IRQ_TIMER3);

    delta_total = 0;

    REG_TM3CNT_L = 0;
    REG_TM3CNT_H = 1 << 7 | 1 << 6;

    return delta_convert_tics(tics);
}


Platform::DeltaClock::~DeltaClock()
{
}


////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////


static volatile u32* keys = (volatile u32*)0x04000130;


void Platform::Keyboard::register_controller(const ControllerInfo& info)
{
    // ...
}


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


static volatile u16* bg0_control = (volatile u16*)0x4000008;
static volatile u16* bg1_control = (volatile u16*)0x400000a;
static volatile u16* bg2_control = (volatile u16*)0x400000c;
static volatile u16* bg3_control = (volatile u16*)0x400000e;


static volatile short* bg0_x_scroll = (volatile short*)0x4000010;
static volatile short* bg0_y_scroll = (volatile short*)0x4000012;
static volatile short* bg1_x_scroll = (volatile short*)0x4000014;
static volatile short* bg1_y_scroll = (volatile short*)0x4000016;
static volatile short* bg2_x_scroll = (volatile short*)0x4000018;
static volatile short* bg2_y_scroll = (volatile short*)0x400001a;
static volatile short* bg3_x_scroll = (volatile short*)0x400001c;
static volatile short* bg3_y_scroll = (volatile short*)0x400001e;


static u8 last_fade_amt;
static ColorConstant last_color;
static bool last_fade_include_sprites;


static volatile u16* reg_blendcnt = (volatile u16*)0x04000050;
static volatile u16* reg_blendalpha = (volatile u16*)0x04000052;

#define BLD_BUILD(top, bot, mode)                                              \
    ((((bot)&63) << 8) | (((mode)&3) << 6) | ((top)&63))
#define BLD_OBJ 0x0010
#define BLD_BG0 0x0001
#define BLD_BG1 0x0002
#define BLD_BG3 0x0008
#define BLDA_BUILD(eva, evb) (((eva)&31) | (((evb)&31) << 8))


#include "gba_color.hpp"


Platform::Screen::Screen() : userdata_(nullptr)
{
    REG_DISPCNT = MODE_0 | OBJ_ENABLE | OBJ_MAP_1D | BG0_ENABLE | BG1_ENABLE |
                  BG2_ENABLE | BG3_ENABLE | WIN0_ENABLE;

    REG_WININ = WIN_ALL;

    // Always display the starfield and the overlay in the outer window. We just
    // want to mask off areas of the game map that have wrapped to the other
    // side of the screen.
    REG_WINOUT = WIN_OBJ | WIN_BG1 | WIN_BG2;

    *reg_blendcnt = BLD_BUILD(BLD_OBJ, BLD_BG0 | BLD_BG1 | BLD_BG3, 0);

    *reg_blendalpha = BLDA_BUILD(0x40 / 8, 0x40 / 8);

    // Tilemap layer 0
    *bg0_control = BG_CBB(cbb_t0_texture) | BG_SBB(sbb_t0_tiles) |
                   BG_REG_64x64 | BG_PRIORITY(3) | BG_MOSAIC;

    // Tilemap layer 1
    *bg3_control = BG_CBB(cbb_t1_texture) | BG_SBB(sbb_t1_tiles) |
                   BG_REG_64x64 | BG_PRIORITY(2) | BG_MOSAIC;

    // The starfield background
    *bg1_control = BG_CBB(cbb_bg_texture) | BG_SBB(sbb_bg_tiles) |
                   BG_PRIORITY(3) | BG_MOSAIC;

    // The overlay
    *bg2_control = BG_CBB(cbb_overlay_texture) | BG_SBB(sbb_overlay_tiles) |
                   BG_PRIORITY(0) | BG_MOSAIC;

    view_.set_size(this->size().cast<Float>());

    REG_MOSAIC = MOS_BUILD(0, 0, 1, 1);
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

    case ColorConstant::cerulean_blue:
        static const Color cerulean_blue(12, 27, 31);
        return cerulean_blue;

    case ColorConstant::picton_blue:
        static const Color picton_blue(9, 20, 31);
        return picton_blue;

    case ColorConstant::maya_blue:
        static const Color maya_blue(10, 23, 31);
        return maya_blue;

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

    case ColorConstant::aged_paper:
        static const Color aged_paper(27, 24, 18);
        return aged_paper;

    case ColorConstant::green:
        static const Color green(5, 24, 21);
        return green;

    case ColorConstant::med_blue_gray:
        static const Color med_blue_gray(14, 14, 21);
        return med_blue_gray;

    case ColorConstant::silver_white:
        static const Color silver_white(29, 29, 30);
        return silver_white;

    case ColorConstant::indigo_tint:
        static const Color indigo_tint(4, 12, 16);
        return indigo_tint;

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


static u8 screen_pixelate_amount = 0;

static bool inverted = false;


void Platform::Screen::invert()
{
    ::inverted = !::inverted;

    for (int i = 0; i < 16; ++i) {
        const auto before = Color::from_bgr_hex_555(MEM_PALETTE[i]);
        MEM_PALETTE[i] = before.invert().bgr_hex_555();
    }

    for (int i = 0; i < 16; ++i) {
        const auto before = Color::from_bgr_hex_555(MEM_BG_PALETTE[i]);
        MEM_BG_PALETTE[i] = before.invert().bgr_hex_555();
    }

    for (int i = 0; i < 16; ++i) {
        const auto before = Color::from_bgr_hex_555(MEM_BG_PALETTE[32 + i]);
        MEM_BG_PALETTE[32 + i] = before.invert().bgr_hex_555();
    }
}


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
    if (UNLIKELY(color_mix_disabled or inverted)) {
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

    if (amount not_eq 255) {
        for (int i = 0; i < 16; ++i) {
            auto from = Color::from_bgr_hex_555(MEM_PALETTE[i]);
            const u32 index = 16 * palette_counter + i;
            MEM_PALETTE[index] = Color(fast_interpolate(c.r_, from.r_, amount),
                                       fast_interpolate(c.g_, from.g_, amount),
                                       fast_interpolate(c.b_, from.b_, amount))
                                     .bgr_hex_555();
        }
    } else {
        for (int i = 0; i < 16; ++i) {
            const u32 index = 16 * palette_counter + i;
            // No need to actually perform the blend operation if we're mixing
            // in 100% of the other color.
            MEM_PALETTE[index] = c.bgr_hex_555();
        }
    }

    palette_info[palette_counter] = {k, amount, true};

    return palette_counter++;
}


void Platform::Screen::draw(const Sprite& spr)
{
    if (UNLIKELY(spr.get_alpha() == Sprite::Alpha::transparent)) {
        return;
    }

    const auto& mix = spr.get_mix();

    const auto pb = [&]() -> PaletteBank {
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

        if ((mix.amount_ > 215 and mix.amount_ < 255) or
            screen_pixelate_amount not_eq 0) {

            oa->attribute_0 |= ATTR0_MOSAIC;
        }

        oa->attribute_1 &= 0xfe00;
        oa->attribute_1 |= (abs_position.x + x_off) & 0x01ff;
        oa->attribute_2 = 2 + spr.get_texture_index() * scale + tex_off;
        oa->attribute_2 |= pb;
        oa->attribute_2 |= ATTR2_PRIORITY(1);
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


static Buffer<Platform::Task*, 8> task_queue_;


void Platform::push_task(Task* task)
{
    task->complete_ = false;
    task->running_ = true;

    if (not task_queue_.push_back(task)) {
        error(*this, "failed to enqueue task");
        while (true)
            ;
    }
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


static void key_wake_isr();
static void key_standby_isr();
static bool enter_sleep = false;


static void key_wake_isr()
{
    REG_KEYCNT = KEY_SELECT | KEY_R | KEY_L | KEYIRQ_ENABLE | KEYIRQ_AND;

    irqSet(IRQ_KEYPAD, key_standby_isr);
}


static void key_standby_isr()
{
    REG_KEYCNT = KEY_SELECT | KEY_START | KEY_A | KEY_B | DPAD | KEYIRQ_ENABLE |
                 KEYIRQ_OR;

    irqSet(IRQ_KEYPAD, key_wake_isr);

    enter_sleep = true;
}


static ScreenBlock overlay_back_buffer alignas(u32);
static bool overlay_back_buffer_changed = false;


void Platform::Screen::display()
{
    // platform->stopwatch().start();

    if (UNLIKELY(enter_sleep)) {
        enter_sleep = false;
        if (not::platform->network_peer().is_connected()) {
            ::platform->sleep(180);
            Stop();
        }
    }

    if (overlay_back_buffer_changed) {
        overlay_back_buffer_changed = false;

        // If the overlay has not scrolled, then we do not need to bother with
        // the lower twelve rows of the overlay tile data, because the screen is
        // twenty tiles tall. This hack could be problematic if someone scrolls
        // the screen a lot without editing the overlay.
        if (overlay_y == 0) {
            memcpy32(MEM_SCREENBLOCKS[sbb_overlay_tiles],
                     overlay_back_buffer,
                     (sizeof(u16) * (21 * 32)) / 4);
        } else {
            memcpy32(MEM_SCREENBLOCKS[sbb_overlay_tiles],
                     overlay_back_buffer,
                     (sizeof overlay_back_buffer) / 4);
        }
    }

    for (u32 i = oam_write_index; i < last_oam_write_index; ++i) {
        object_attribute_back_buffer[i].attribute_0 |= attr0_mask::disabled;

        // object_attribute_back_buffer[i].attribute_2 = ATTR2_PRIORITY(3);
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

    *bg3_x_scroll = view_offset.x;
    *bg3_y_scroll = view_offset.y;

    // Depending on the amount of the background scroll, we want to mask off
    // certain parts of bg0 and bg3. The background tiles wrap when they scroll
    // a certain distance, and wrapping looks strange (although it might be
    // useful if you were making certain kinds of games, like some kind of
    // Civilization clone, but for BlindJump, it doesn't make sense to display
    // the wrapped area).
    const s32 scroll_limit_x_max = 512 - size().x;
    const s32 scroll_limit_y_max = 480 - size().y;
    if (view_offset.x > scroll_limit_x_max) {
        REG_WIN0H =
            (0 << 8) | (size().x - (view_offset.x - scroll_limit_x_max));
    } else if (view_offset.x < 0) {
        REG_WIN0H = ((view_offset.x * -1) << 8) | (0);
    } else {
        REG_WIN0H = (0 << 8) | (size().x);
    }

    if (view_offset.y > scroll_limit_y_max) {
        REG_WIN0V =
            (0 << 8) | (size().y - (view_offset.y - scroll_limit_y_max));
    } else if (view_offset.y < 0) {
        REG_WIN0V = ((view_offset.y * -1) << 8) | (0);
    } else {
        REG_WIN0V = (0 << 8) | (size().y);
    }

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


#include "data/blaster_info_flattened.h"
#include "data/charset_en_spn_fr.h"
#include "data/launch_flattened.h"
#include "data/old_poster_flattened.h"
#include "data/overlay.h"
#include "data/overlay_cutscene.h"
#include "data/overlay_journal.h"
#include "data/overlay_network_flattened.h"
#include "data/seed_packet_flattened.h"
#include "data/spritesheet.h"
#include "data/spritesheet2.h"
#include "data/spritesheet3.h"
#include "data/spritesheet_boss0.h"
#include "data/spritesheet_boss1.h"
#include "data/spritesheet_intro_clouds.h"
#include "data/spritesheet_intro_cutscene.h"
#include "data/spritesheet_launch_anim.h"
#include "data/tilesheet.h"
#include "data/tilesheet2.h"
#include "data/tilesheet2_top.h"
#include "data/tilesheet3.h"
#include "data/tilesheet3_top.h"
#include "data/tilesheet_intro_cutscene_flattened.h"
#include "data/tilesheet_top.h"


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
    TEXTURE_INFO(spritesheet_intro_cutscene),
    TEXTURE_INFO(spritesheet_intro_clouds),
    TEXTURE_INFO(spritesheet),
    TEXTURE_INFO(spritesheet2),
    TEXTURE_INFO(spritesheet3),
    TEXTURE_INFO(spritesheet_boss0),
    TEXTURE_INFO(spritesheet_boss1),
    TEXTURE_INFO(spritesheet_launch_anim)};


static const TextureData tile_textures[] = {
    TEXTURE_INFO(tilesheet),
    TEXTURE_INFO(tilesheet_top),
    TEXTURE_INFO(tilesheet2),
    TEXTURE_INFO(tilesheet2_top),
    TEXTURE_INFO(tilesheet3),
    TEXTURE_INFO(tilesheet3_top),
    TEXTURE_INFO(tilesheet_intro_cutscene_flattened),
    TEXTURE_INFO(launch_flattened)};


static const TextureData overlay_textures[] = {
    TEXTURE_INFO(charset_en_spn_fr),
    TEXTURE_INFO(overlay),
    TEXTURE_INFO(overlay_cutscene),
    TEXTURE_INFO(overlay_journal),
    TEXTURE_INFO(old_poster_flattened),
    TEXTURE_INFO(blaster_info_flattened),
    TEXTURE_INFO(seed_packet_flattened),
    TEXTURE_INFO(overlay_network_flattened)};


static const TextureData* current_spritesheet = &sprite_textures[0];
static const TextureData* current_tilesheet0 = &tile_textures[0];
static const TextureData* current_tilesheet1 = &tile_textures[1];
static const TextureData* current_overlay_texture = &overlay_textures[1];

static u16 sprite_palette[16];
static u16 tilesheet_0_palette[16];
static u16 tilesheet_1_palette[16];


static Contrast contrast = 0;


Contrast Platform::Screen::get_contrast() const
{
    return ::contrast;
}


static void init_palette(const TextureData* td, u16* palette)
{
    for (int i = 0; i < 16; ++i) {

        if (::contrast not_eq 0) {
            const Float f =
                (259.f * (contrast + 255)) / (255 * (259 - contrast));
            const auto c = Color::from_bgr_hex_555(td->palette_data_[i]);

            const auto r =
                clamp(f * (Color::upsample(c.r_) - 128) + 128, 0.f, 255.f);
            const auto g =
                clamp(f * (Color::upsample(c.g_) - 128) + 128, 0.f, 255.f);
            const auto b =
                clamp(f * (Color::upsample(c.b_) - 128) + 128, 0.f, 255.f);

            palette[i] = Color(Color::downsample(r),
                               Color::downsample(g),
                               Color::downsample(b))
                             .bgr_hex_555();

        } else {
            palette[i] = td->palette_data_[i];
        }
    }
}


void Platform::Screen::set_contrast(Contrast c)
{
    ::contrast = c;

    init_palette(current_spritesheet, sprite_palette);
    init_palette(current_tilesheet0, tilesheet_0_palette);
    init_palette(current_tilesheet1, tilesheet_1_palette);
}


static bool validate_tilemap_texture_size(Platform& pfrm, size_t size)
{
    constexpr auto charblock_size = sizeof(ScreenBlock) * 7;
    if (size > charblock_size) {
        error(pfrm, "tileset exceeds charblock size");
        pfrm.fatal();
        return false;
    }
    return true;
}


static bool validate_overlay_texture_size(Platform& pfrm, size_t size)
{
    constexpr auto charblock_size = sizeof(ScreenBlock) * 8;
    if (size > charblock_size) {
        error(pfrm, "tileset exceeds charblock size");
        pfrm.fatal();
        return false;
    }
    return true;
}


COLD static void set_map_tile(u8 base, u16 x, u16 y, u16 tile_id, int palette)
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
            MEM_SCREENBLOCKS[base + 1][i + ref(x % 8, y)] =
                (tile_id * 12 + i) | SE_PALBANK(palette);
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[base + 1][i + ref(x % 8, y) + 32] =
                (tile_id * 12 + i + 4) | SE_PALBANK(palette);
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[base + 3][i + ref(x % 8, 0)] =
                (tile_id * 12 + i + 8) | SE_PALBANK(palette);
        }
        return;
    } else if (y == 10) {
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[base][i + ref(x, y)] =
                (tile_id * 12 + i) | SE_PALBANK(palette);
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[base][i + ref(x, y) + 32] =
                (tile_id * 12 + i + 4) | SE_PALBANK(palette);
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[base + 2][i + ref(x, 0)] =
                (tile_id * 12 + i + 8) | SE_PALBANK(palette);
        }
        return;
    }

    auto screen_block = [&]() -> u16 {
        if (x > 7 and y > 9) {
            x %= 8;
            y %= 10;
            return base + 3;
        } else if (y > 9) {
            y %= 10;
            return base + 2;
        } else if (x > 7) {
            x %= 8;
            return base + 1;
        } else {
            return base;
        }
    }();

    if (screen_block == base + 2 or screen_block == base + 3) {
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y - 1) + 32] =
                (tile_id * 12 + i) | SE_PALBANK(palette);
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y - 1) + 64] =
                (tile_id * 12 + i + 4) | SE_PALBANK(palette);
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y - 1) + 96] =
                (tile_id * 12 + i + 8) | SE_PALBANK(palette);
        }
    } else {
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y)] =
                (tile_id * 12 + i) | SE_PALBANK(palette);
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y) + 32] =
                (tile_id * 12 + i + 4) | SE_PALBANK(palette);
        }
        for (u32 i = 0; i < 4; ++i) {
            MEM_SCREENBLOCKS[screen_block][i + ref(x, y) + 64] =
                (tile_id * 12 + i + 8) | SE_PALBANK(palette);
        }
    }
}


static u16 get_map_tile(u8 base, u16 x, u16 y, int palette)
{
    // I know that this code is confusing, sorry! See comment in set_map_tile().

    auto ref = [](u16 x_, u16 y_) { return x_ * 4 + y_ * 32 * 3; };

    if (y == 10 and x > 7) {
        return (MEM_SCREENBLOCKS[base][ref(x % 8, y)] &
                ~(SE_PALBANK(palette))) /
               12;
    } else if (y == 10) {
        return (MEM_SCREENBLOCKS[base][ref(x, y)] & ~(SE_PALBANK(palette))) /
               12;
    }

    auto screen_block = [&]() -> u16 {
        if (x > 7 and y > 9) {
            x %= 8;
            y %= 10;
            return base + 3;
        } else if (y > 9) {
            y %= 10;
            return base + 2;
        } else if (x > 7) {
            x %= 8;
            return base + 1;
        } else {
            return base;
        }
    }();

    if (screen_block == base + 2 or screen_block == base + 3) {
        return (MEM_SCREENBLOCKS[screen_block][ref(x, y - 1) + 32] &
                ~(SE_PALBANK(palette))) /
               12;
    } else {
        return (MEM_SCREENBLOCKS[screen_block][ref(x, y)] &
                ~(SE_PALBANK(palette))) /
               12;
    }
}


u16 Platform::get_tile(Layer layer, u16 x, u16 y)
{
    switch (layer) {
    case Layer::overlay:
        if (x > 31 or y > 31) {
            return 0;
        }
        return overlay_back_buffer[x + y * 32] & ~(SE_PALBANK_MASK);

    case Layer::background:
        if (x > 31 or y > 31) {
            return 0;
        }
        return MEM_SCREENBLOCKS[sbb_bg_tiles][x + y * 32];

    case Layer::map_0:
        return get_map_tile(sbb_t0_tiles, x, y, 0);

    case Layer::map_1:
        return get_map_tile(sbb_t1_tiles, x, y, 2);
    }
    return 0;
}


void Platform::fatal()
{
    SoftReset(ROM_RESTART), __builtin_unreachable();
}


void Platform::set_overlay_origin(Float x, Float y)
{
    *bg2_x_scroll = static_cast<s16>(x);
    *bg2_y_scroll = static_cast<s16>(y);
    overlay_y = y;
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


// Screen fades are cpu intensive. We want to skip any work that we possibly
// can.
static bool overlay_was_faded = false;


// TODO: May be possible to reduce tearing by deferring the fade until the
// Screen::display() call...
void Platform::Screen::fade(float amount,
                            ColorConstant k,
                            std::optional<ColorConstant> base,
                            bool include_sprites,
                            bool include_overlay)
{
    ::inverted = false;

    const u8 amt = amount * 255;

    if (amt < 128) {
        color_mix_disabled = false;
    } else {
        color_mix_disabled = true;
    }

    if (amt == last_fade_amt and k == last_color and
        last_fade_include_sprites == include_sprites) {
        return;
    }

    last_fade_amt = amt;
    last_color = k;
    last_fade_include_sprites = include_sprites;

    const auto& c = real_color(k);

    if (not base) {
        for (int i = 0; i < 16; ++i) {
            auto from = Color::from_bgr_hex_555(sprite_palette[i]);
            MEM_PALETTE[i] = blend(from, c, include_sprites ? amt : 0);
        }
        for (int i = 0; i < 16; ++i) {
            auto from = Color::from_bgr_hex_555(tilesheet_0_palette[i]);
            MEM_BG_PALETTE[i] = blend(from, c, amt);
        }
        for (int i = 0; i < 16; ++i) {
            auto from = Color::from_bgr_hex_555(tilesheet_1_palette[i]);
            MEM_BG_PALETTE[32 + i] = blend(from, c, amt);
        }
        if (include_overlay or overlay_was_faded) {
            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(
                    current_overlay_texture->palette_data_[i]);
                MEM_BG_PALETTE[16 + i] =
                    blend(from, c, include_overlay ? amt : 0);
            }
        }
        overlay_was_faded = include_overlay;
    } else {
        for (int i = 0; i < 16; ++i) {
            MEM_PALETTE[i] =
                blend(real_color(*base), c, include_sprites ? amt : 0);
            MEM_BG_PALETTE[i] = blend(real_color(*base), c, amt);
            MEM_BG_PALETTE[32 + i] = blend(real_color(*base), c, amt);

            if (overlay_was_faded) {
                // FIXME!
                for (int i = 0; i < 16; ++i) {
                    auto from = Color::from_bgr_hex_555(
                        current_overlay_texture->palette_data_[i]);
                    MEM_BG_PALETTE[16 + i] = blend(from, c, 0);
                }
                overlay_was_faded = false;
            }
        }
    }
}


void Platform::Screen::pixelate(u8 amount,
                                bool include_overlay,
                                bool include_background,
                                bool include_sprites)
{
    screen_pixelate_amount = amount;

    if (amount == 0) {
        REG_MOSAIC = MOS_BUILD(0, 0, 1, 1);
    } else {
        REG_MOSAIC = MOS_BUILD(amount >> 4,
                               amount >> 4,
                               include_sprites ? amount >> 4 : 0,
                               include_sprites ? amount >> 4 : 0);

        if (include_overlay) {
            *bg2_control |= BG_MOSAIC;
        } else {
            *bg2_control &= ~BG_MOSAIC;
        }

        if (include_background) {
            *bg0_control |= BG_MOSAIC;
            *bg1_control |= BG_MOSAIC;
        } else {
            *bg0_control &= ~BG_MOSAIC;
            *bg1_control &= ~BG_MOSAIC;
        }
    }
}


void Platform::load_sprite_texture(const char* name)
{
    for (auto& info : sprite_textures) {

        if (strcmp(name, info.name_) == 0) {

            current_spritesheet = &info;

            init_palette(current_spritesheet, sprite_palette);


            // NOTE: There are four tile blocks, so index four points to the
            // end of the tile memory.
            memcpy16((void*)&MEM_TILE[4][1],
                     info.tile_data_,
                     info.tile_data_length_ / 2);

            // We need to do this, otherwise whatever screen fade is currently
            // active will be overwritten by the copy.
            const auto& c = real_color(last_color);
            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(sprite_palette[i]);
                MEM_PALETTE[i] = blend(from, c, last_fade_amt);
            }
        }
    }
}


void Platform::load_tile0_texture(const char* name)
{
    for (auto& info : tile_textures) {

        if (strcmp(name, info.name_) == 0) {

            current_tilesheet0 = &info;

            init_palette(current_tilesheet0, tilesheet_0_palette);


            // We don't want to load the whole palette into memory, we might
            // overwrite palettes used by someone else, e.g. the overlay...
            //
            // Also, like the sprite texture, we need to apply the currently
            // active screen fade while modifying the color palette.
            const auto& c = real_color(last_color);
            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(tilesheet_0_palette[i]);
                MEM_BG_PALETTE[i] = blend(from, c, last_fade_amt);
            }

            if (validate_tilemap_texture_size(*this, info.tile_data_length_)) {
                memcpy16((void*)&MEM_SCREENBLOCKS[sbb_t0_texture][0],
                         info.tile_data_,
                         info.tile_data_length_ / 2);
            } else {
                StringBuffer<45> buf = "unable to load: ";
                buf += name;

                error(*this, buf.c_str());
            }
        }
    }
}


void Platform::load_tile1_texture(const char* name)
{
    for (auto& info : tile_textures) {

        if (strcmp(name, info.name_) == 0) {

            current_tilesheet1 = &info;

            init_palette(current_tilesheet1, tilesheet_1_palette);


            // We don't want to load the whole palette into memory, we might
            // overwrite palettes used by someone else, e.g. the overlay...
            //
            // Also, like the sprite texture, we need to apply the currently
            // active screen fade while modifying the color palette.
            const auto& c = real_color(last_color);
            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(tilesheet_1_palette[i]);
                MEM_BG_PALETTE[32 + i] = blend(from, c, last_fade_amt);
                tilesheet_1_palette[i] = info.palette_data_[i];
            }

            if (validate_tilemap_texture_size(*this, info.tile_data_length_)) {
                memcpy16((void*)&MEM_SCREENBLOCKS[sbb_t1_texture][0],
                         info.tile_data_,
                         info.tile_data_length_ / 2);
            } else {
                StringBuffer<45> buf = "unable to load: ";
                buf += name;

                error(*this, buf.c_str());
            }
        }
    }
}


void Platform::sleep(u32 frames)
{
    // NOTE: A sleep call should just pause the game for some number of frames,
    // but doing so should not have an impact on delta timing
    // calculation. Therefore, we need to stop the hardware timer associated
    // with the delta clock, and zero out the clock's contents when finished
    // with the sleep cycles.

    irqDisable(IRQ_TIMER3);

    while (frames--) {
        VBlankIntrWait();
    }

    irqEnable(IRQ_TIMER3);
}


bool Platform::is_running() const
{
    // Unlike the windowed desktop platform, as long as the device is
    // powered on, the game is running.
    return true;
}


static byte* const cartridge_ram = (byte*)0x0E000000;


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
            *(volatile u8*)0x0E005555 = 0xAA;
            *(volatile u8*)0x0E002AAA = 0x55;
            *(volatile u8*)0x0E005555 = 0xA0;
        }
        *dst++ = *src++;
    }
}


static void set_flash_bank(u32 bankID)
{
    if (bankID < 2) {
        *(volatile u8*)0x0E005555 = 0xAA;
        *(volatile u8*)0x0E002AAA = 0x55;
        *(volatile u8*)0x0E005555 = 0xB0;
        *(volatile u8*)0x0E000000 = bankID;
    }
}

// FIXME: Lets be nice to the flash an not write to the same memory
// location every single time! What about a list? Each new save will
// have a unique id. On startup, scan through memory until you reach
// the highest unique id. Then start writing new saves at that
// point. NOTE: My everdrive uses SRAM for Flash writes anyway, so
// it's probably not going to wear out, but I like to pretend that I'm
// developing a real gba game.


COLD static bool flash_save(const void* data, u32 flash_offset, u32 length)
{
    if ((u32)flash_offset >= 0x10000) {
        set_flash_bank(1);
    } else {
        set_flash_bank(0);
    }

    flash_bytecpy((void*)(cartridge_ram + flash_offset), data, length, true);

    return flash_byteverify(
        (void*)(cartridge_ram + flash_offset), data, length);
}


static void flash_load(void* dest, u32 flash_offset, u32 length)
{
    if (flash_offset >= 0x10000) {
        set_flash_bank(1);
    } else {
        set_flash_bank(0);
    }

    flash_bytecpy(
        dest, (const void*)(cartridge_ram + flash_offset), length, false);
}


static bool save_using_flash = false;


void sram_save(const void* data, u32 offset, u32 length)
{
    u8* save_mem = (u8*)cartridge_ram + offset;

    // The cartridge has an 8-bit bus, so you have to write one byte at a time,
    // otherwise it won't work!
    for (size_t i = 0; i < length; ++i) {
        *save_mem++ = ((const u8*)data)[i];
    }
}


void sram_load(void* dest, u32 offset, u32 length)
{
    u8* save_mem = (u8*)cartridge_ram + offset;
    for (size_t i = 0; i < length; ++i) {
        ((u8*)dest)[i] = *save_mem++;
    }
}


bool Platform::write_save_data(const void* data, u32 length)
{
    if (save_using_flash) {
        return flash_save(data, 0, length);
    } else {
        sram_save(data, 0, length);
        return true;
    }
}


bool Platform::read_save_data(void* buffer, u32 data_length)
{
    if (save_using_flash) {
        flash_load(buffer, 0, data_length);
    } else {
        sram_load(buffer, 0, data_length);
    }
    return true;
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


// This is unfortunate. Maybe we should define a max save data size as part of
// the platform header, so that we do not need to pull in game specific code.
#include "persistentData.hpp"


// NOTE: PersistentData goes first into flash memory, followed by the game's
// logs. The platform implementation isn't supposed to need to know about the
// layout of the game's save data, but, in this particular implementation, we're
// using the cartridge ram as a logfile.
static const u32 initial_log_write_loc = sizeof(PersistentData);
static u32 log_write_loc = initial_log_write_loc;


Platform::Logger::Logger()
{
}


void Platform::Logger::log(Logger::Severity level, const char* msg)
{
    std::array<char, 256> buffer;

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

    if (log_write_loc + prefix_size + msg_size + 2 >= 64000) {
        // We cannot be certain of how much cartridge ram will be available. But
        // 64k is a reasonable assumption. When we reach the end, wrap around to
        // the beginning.
        log_write_loc = initial_log_write_loc;
    }


    for (i = 0;
         i < std::min(size_t(msg_size), buffer.size() - (prefix_size + 1));
         ++i) {
        buffer[i + 3] = msg[i];
    }
    buffer[i + 3] = '\n';
    buffer[i + 4] = '\n'; // This char will be overwritten, meant to identify
                          // the end of the log, in the case where the log wraps
                          // around.

    if (save_using_flash) {
        flash_save(buffer.data(), log_write_loc, buffer.size());
    } else {
        sram_save(buffer.data(), log_write_loc, buffer.size());
    }

    log_write_loc += msg_size + prefix_size + 1;
}


void Platform::Logger::read(void* buffer, u32 start_offset, u32 num_bytes)
{
    if (save_using_flash) {
        flash_load(buffer, sizeof(PersistentData) + start_offset, num_bytes);
    } else {
        sram_load(buffer, sizeof(PersistentData) + start_offset, num_bytes);
    }
}


////////////////////////////////////////////////////////////////////////////////
// Speaker
//
// For music, the Speaker class uses the GameBoy's direct sound chip to play
// 8-bit signed raw audio, at 16kHz.
//
////////////////////////////////////////////////////////////////////////////////


void Platform::Speaker::play_note(Note n, Octave o, Channel c)
{
}


#include "data/clair_de_lune.hpp"
#include "data/scottbuckley_computations.hpp"
#include "data/scottbuckley_hiraeth.hpp"
#include "data/scottbuckley_omega.hpp"
#include "data/september.hpp"


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
} music_tracks[] = {
    DEF_AUDIO(hiraeth, scottbuckley_hiraeth),
    DEF_AUDIO(omega, scottbuckley_omega),
    DEF_AUDIO(computations, scottbuckley_computations),
    DEF_AUDIO(clair_de_lune, clair_de_lune)
    // DEF_AUDIO(september, september)
};


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
#include "data/sound_bell.hpp"
#include "data/sound_blaster.hpp"
#include "data/sound_click.hpp"
#include "data/sound_coin.hpp"
#include "data/sound_creak.hpp"
#include "data/sound_explosion1.hpp"
#include "data/sound_explosion2.hpp"
#include "data/sound_footstep1.hpp"
#include "data/sound_footstep2.hpp"
#include "data/sound_footstep3.hpp"
#include "data/sound_footstep4.hpp"
#include "data/sound_heart.hpp"
#include "data/sound_laser1.hpp"
#include "data/sound_open_book.hpp"
#include "data/sound_openbag.hpp"
#include "data/sound_pop.hpp"


static const AudioTrack sounds[] = {DEF_AUDIO(explosion1, sound_explosion1),
                                    DEF_AUDIO(explosion1, sound_explosion2),
                                    DEF_AUDIO(footstep1, sound_footstep1),
                                    DEF_AUDIO(footstep2, sound_footstep2),
                                    DEF_AUDIO(footstep3, sound_footstep3),
                                    DEF_AUDIO(footstep4, sound_footstep4),
                                    DEF_AUDIO(open_book, sound_open_book),
                                    DEF_AUDIO(openbag, sound_openbag),
                                    DEF_AUDIO(blaster, sound_blaster),
                                    DEF_AUDIO(laser1, sound_laser1),
                                    DEF_AUDIO(creak, sound_creak),
                                    DEF_AUDIO(heart, sound_heart),
                                    DEF_AUDIO(click, sound_click),
                                    DEF_AUDIO(coin, sound_coin),
                                    DEF_AUDIO(bell, sound_bell),
                                    DEF_AUDIO(pop, sound_pop)};


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


void Platform::Speaker::set_position(const Vec2<Float>&)
{
    // We don't support spatialized audio on the gameboy.
}


void Platform::Speaker::play_sound(const char* name,
                                   int priority,
                                   std::optional<Vec2<Float>> position)
{
    (void)position; // We're not using position data, because on the gameboy
                    // advance, we aren't supporting spatial audio.

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


static void play_music(const char* name, bool loop, Microseconds offset)
{
    const auto track = find_track(name);
    if (track == nullptr) {
        return;
    }

    const Microseconds sample_offset = offset * 0.016f; // NOTE: because 16kHz

    modify_audio([&] {
        snd_ctx.music_track_length = track->length_;
        snd_ctx.music_track_loop = loop;
        snd_ctx.music_track = track->data_;
        snd_ctx.music_track_pos = sample_offset % track->length_;
    });
}


void Platform::Speaker::play_music(const char* name,
                                   bool loop,
                                   Microseconds offset)
{
    // NOTE: The sound sample needs to be mono, and 8-bit signed. To export this
    // format from Audacity, convert the tracks to mono via the Tracks dropdown,
    // and then export as raw, in the format 8-bit signed.
    //
    // Also, important to convert the sound file frequency to 16kHz.

    this->stop_music();

    ::play_music(name, loop, offset);

    // FIXME!!!!!! Mysteriously, there's a weird audio glitch, where the sound
    // effects, but not the music, get all glitched out until two sounds are
    // played consecutively. I've spent hours trying to figure out what's going
    // wrong, and I haven't solved this one yet, so for now, just play a couple
    // quiet sounds. To add further confusion, after adjusting the instruction
    // prefetch and waitstats, I need to play three sounds
    // consecutively... obviously my interrupt service routine for the audio is
    // flawed somehow. Do I need to completely disable the timers and sound
    // chip, as well as the audio interrupts, when playing new sounds? Does
    // disabling the audio interrupts when queueing a new sound effect cause
    // audio artifacts, because the sound chip is not receiving samples?
    play_sound("footstep1", 0);
    play_sound("footstep2", 0);
    play_sound("footstep3", 0);
}


Platform::Speaker::Speaker()
{
}


////////////////////////////////////////////////////////////////////////////////
// Misc
////////////////////////////////////////////////////////////////////////////////


#define REG_SGFIFOA *(volatile u32*)0x40000A0


// NOTE: I tried to move this audio update interrupt handler to IWRAM, but the
// sound output became sort of glitchy, and I noticed some tearing in the
// display. At the same time, the game was less laggy, so maybe when I work out
// the kinks, this function will eventually be moved to arm code instead of
// thumb.
static void audio_update_isr()
{
    alignas(4) AudioSample mixing_buffer[4];

    if (snd_ctx.music_track) {

        for (int i = 0; i < 4; ++i) {
            // FIXME: Is the stuff stored in the data segment guaranteed to be
            // word-aligned? If so, we can potentially copy faster by casting to
            // u32.
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


void Platform::soft_exit()
{
    Stop();
}


static Microseconds watchdog_counter;


static std::optional<Platform::WatchdogCallback> watchdog_callback;


static void watchdog_update_isr()
{
    // NOTE: The watchdog timer is configured to have a period of 61.04
    // microseconds. 0xffff is the max counter value upon overflow.
    ::watchdog_counter += 61 * 0xffff;

    if (::watchdog_counter > seconds(10)) {
        ::watchdog_counter = 0;

        if (::platform and ::watchdog_callback) {
            (*::watchdog_callback)(*platform);
        }

        SoftReset(ROM_RESTART);
    }
}


void Platform::feed_watchdog()
{
    ::watchdog_counter = 0;
}


void Platform::on_watchdog_timeout(WatchdogCallback callback)
{
    ::watchdog_callback.emplace(callback);
}


extern const unsigned char config_ini[];


const char* Platform::config_data() const
{
    return (const char*)config_ini;
}


__attribute__((section(".iwram"), long_call)) void
cartridge_interrupt_handler();


static void enable_watchdog()
{
    irqEnable(IRQ_TIMER2);
    irqSet(IRQ_TIMER2, watchdog_update_isr);

    REG_TM2CNT_H = 0x00C3;
    REG_TM2CNT_L = 0;
}


bool use_optimized_waitstates = false;


// EWRAM is large, but has a narrower bus. The platform offers a window into
// EWRAM, called scratch space, for non-essential stuff. Right now, I am setting
// the buffer to ~100K in size. One could theoretically make the buffer almost
// 256kB, because I am using none of EWRAM as far as I know...
static EWRAM_DATA
    ObjectPool<RcBase<Platform::ScratchBuffer,
                      Platform::scratch_buffer_count>::ControlBlock,
               Platform::scratch_buffer_count>
        scratch_buffer_pool;


static int scratch_buffers_in_use = 0;


Platform::ScratchBufferPtr Platform::make_scratch_buffer()
{
    auto finalizer = [](RcBase<Platform::ScratchBuffer,
                               scratch_buffer_count>::ControlBlock* ctrl) {
        --scratch_buffers_in_use;
        ctrl->pool_->post(ctrl);
    };

    auto maybe_buffer = Rc<ScratchBuffer, scratch_buffer_count>::create(
        &scratch_buffer_pool, finalizer);
    if (maybe_buffer) {
        ++scratch_buffers_in_use;
        return *maybe_buffer;
    } else {
        screen().fade(1.f, ColorConstant::electric_blue);
        error(*this, "scratch buffer pool exhausted");
        fatal();
    }
}


int Platform::scratch_buffers_remaining()
{
    return scratch_buffer_count - scratch_buffers_in_use;
}


Platform::~Platform()
{
    // ...
}


struct GlyphMapping {
    utf8::Codepoint character_;

    // -1 represents unassigned. Mapping a tile into memory sets the reference
    //  count to zero. When a call to Platform::set_tile reduces the reference
    //  count back to zero, the tile is once again considered to be unassigned,
    //  and will be set to -1.
    s16 reference_count_ = -1;

    bool valid() const
    {
        return reference_count_ > -1;
    }
};


static constexpr const auto glyph_start_offset = 1;
static constexpr const auto glyph_mapping_count = 78;

struct GlyphTable {
    GlyphMapping mappings_[glyph_mapping_count];
};

static std::optional<ScratchBufferBulkAllocator> glyph_table_mem;
static ScratchBufferBulkAllocator::Ptr<GlyphTable> glyph_table =
    ScratchBufferBulkAllocator::null<GlyphTable>();


static void audio_start()
{
    REG_SOUNDCNT_H =
        0x0B0F; //DirectSound A + fifo reset + max volume to L and R
    REG_SOUNDCNT_X = 0x0080; //turn sound chip on

    irqEnable(IRQ_TIMER1);
    irqSet(IRQ_TIMER1, audio_update_isr);

    REG_TM0CNT_L = 0xffff;
    REG_TM1CNT_L = 0xffff - 3; // I think that this is correct, but I'm not
                               // certain... so we want to play four samples at
                               // a time, which means that by subtracting three
                               // from the initial count, the timer will
                               // overflow at the correct rate, right?

    // While it may look like TM0 is unused, it is in fact used for setting the
    // sample rate for the digital audio chip.
    REG_TM0CNT_H = 0x0083;
    REG_TM1CNT_H = 0x00C3;
}


// We want our code to be resiliant to cartridges lacking an RTC chip. Run the
// timer-based delta clock for a while, and make sure that the RTC also counted
// up.
static bool rtc_verify_operability(Platform& pfrm)
{
    Microseconds counter = 0;

    const auto tm1 = pfrm.system_clock().now();

    while (counter < seconds(1) + milliseconds(250)) {
        counter += pfrm.delta_clock().reset();
    }

    const auto tm2 = pfrm.system_clock().now();

    return tm1 and tm2 and time_diff(*tm1, *tm2) > 0;
}


static bool rtc_faulty = false;


static std::optional<DateTime> start_time;


std::optional<DateTime> Platform::startup_time() const
{
    return ::start_time;
}


Platform::Platform()
{
    // Not sure how else to determine whether the cartridge has sram, flash, or
    // something else. An sram write will fail if the cartridge ram is flash, so
    // attempt to save, and if the save fails, assume flash. I don't really know
    // anything about the EEPROM hardware interface...
    static const int sram_test_const = 0xAAAAAAAA;
    sram_save(&sram_test_const, log_write_loc, sizeof sram_test_const);

    int sram_test_result = 0;
    sram_load(&sram_test_result, log_write_loc, sizeof sram_test_result);

    if (sram_test_result not_eq sram_test_const) {
        save_using_flash = true;
        info(*this, "SRAM write failed, falling back to FLASH");
    }

    glyph_table_mem.emplace(*this);
    glyph_table = glyph_table_mem->alloc<GlyphTable>();
    if (not glyph_table) {
        error(*this, "failed to allocate glyph table");
        fatal();
    }

    // IMPORTANT: No calls to map_glyph() are allowed before reaching this
    // line. Otherwise, the glyph table has not yet been constructed.

    info(*this, "Verifying BIOS...");

    switch (BiosCheckSum()) {
    case -1162995584:
        info(*this, "BIOS matches Nintendo DS");
        break;
    case -1162995585:
        info(*this, "BIOS matches GAMEBOY Advance");
        break;
    default:
        warning(*this, "BIOS checksum failed, may be corrupt");
        break;
    }

    // NOTE: Non-sequential 8 and sequential 3 seem to work well for Cart 0 wait
    // states, although setting these options unmasks a few obscure audio bugs,
    // the game displays visibly less tearing. The cartridge prefetch unmasks
    // even more aggressive audio bugs, and doesn't seem to grant obvious
    // performance benefits, so I'm leaving the cartridge prefetch turned off...
    if (use_optimized_waitstates) {
        // Although there is less tearing when running with optimized
        // waitstates, I actually prefer the feature turned off. I really tuned
        // the feel of the controls before I knew about waitstates, and
        // something just feels off to me when turning this feature on. The game
        // is almost too smooth.
        REG_WAITCNT = 0b0000001100010111;
        info(*this, "enabled optimized waitstates...");
    }

    // NOTE: initializing the system clock is easier before interrupts are
    // enabled, because the system clock pulls data from the gpio port on the
    // cartridge.
    system_clock_.init(*this);

    irqInit();
    irqEnable(IRQ_VBLANK);

    irqEnable(IRQ_KEYPAD);
    key_wake_isr();

    irqSet(IRQ_TIMER3, [] {
        delta_total += 0xffff;

        REG_TM3CNT_H = 0;
        REG_TM3CNT_L = 0;
        REG_TM3CNT_H = 1 << 7 | 1 << 6;
    });

    if ((rtc_faulty = not rtc_verify_operability(*this))) {
        info(*this, "RTC chip appears either non-existant or non-functional");
    } else {
        ::start_time = system_clock_.now();
    }

    // Surprisingly, the default value of SIOCNT is not necessarily zero! The
    // source of many past serial comms headaches...
    REG_SIOCNT = 0;


    fill_overlay(0);

    audio_start();

    enable_watchdog();

    irqEnable(IRQ_GAMEPAK);
    irqSet(IRQ_GAMEPAK, cartridge_interrupt_handler);

    for (u32 i = 0; i < Screen::sprite_limit; ++i) {
        // This was a really insidious bug to track down! When failing to hide
        // unused attributes in the back buffer, the uninitialized objects punch
        // a 1 tile (8x8 pixel) hole in the top left corner of the overlay
        // layer, but not exactly. The tile in the high priority background
        // layer still shows up, but lower priority sprites show through the top
        // left tile, I guess I'm observing some weird interaction involving an
        // overlap between a priority 0 tile and a priority 1 sprite: when a
        // priority 1 sprite is sandwitched in between the two tile layers, the
        // priority 0 background tiles seems to be drawn behind the priority 1
        // sprite. I have no idea why!
        object_attribute_back_buffer[i].attribute_2 = ATTR2_PRIORITY(3);
        object_attribute_back_buffer[i].attribute_0 |= attr0_mask::disabled;
    }
}


static bool glyph_mode = false;


void Platform::enable_glyph_mode(bool enabled)
{
    if (enabled) {
        for (auto& gm : ::glyph_table->mappings_) {
            gm.reference_count_ = -1;
        }
    }
    glyph_mode = enabled;
}


void Platform::load_overlay_texture(const char* name)
{
    for (auto& info : overlay_textures) {

        if (strcmp(name, info.name_) == 0) {

            current_overlay_texture = &info;

            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(info.palette_data_[i]);
                if (not overlay_was_faded) {
                    MEM_BG_PALETTE[16 + i] = from.bgr_hex_555();
                } else {
                    const auto& c = real_color(last_color);
                    MEM_BG_PALETTE[16 + i] = blend(from, c, last_fade_amt);
                }
            }

            if (validate_overlay_texture_size(*this, info.tile_data_length_)) {
                memcpy16((void*)&MEM_SCREENBLOCKS[sbb_overlay_texture][0],
                         info.tile_data_,
                         info.tile_data_length_ / 2);
            }

            if (glyph_mode) {
                for (auto& gm : ::glyph_table->mappings_) {
                    gm.reference_count_ = -1;
                }
            }
        }
    }
}


static const TileDesc bad_glyph = 111;


static constexpr int vram_tile_size()
{
    // 8 x 8 x (4 bitsperpixel / 8 bitsperbyte)
    return 32;
}


// Rather than doing tons of extra work to keep the palettes
// coordinated between different image files, use tile index
// 81 as a registration block, which holds a set of colors
// to use when mapping glyphs into vram.
static u8* font_index_tile()
{
    return (u8*)&MEM_SCREENBLOCKS[sbb_overlay_texture][0] +
           ((81) * vram_tile_size());
}


struct FontColorIndices {
    int fg_;
    int bg_;
};


static FontColorIndices font_color_indices()
{
    const auto index = font_index_tile();
    return {index[0] & 0x0f, index[1] & 0x0f};
}


// This code uses a lot of naive algorithms for searching the glyph mapping
// table, potentially could be sped up, but we don't want to use any extra
// memory, we've only got 256K to work with, and the table is big enough as it
// is.
TileDesc Platform::map_glyph(const utf8::Codepoint& glyph,
                             TextureCpMapper mapper)
{
    if (not glyph_mode) {
        return bad_glyph;
    }

    for (TileDesc tile = 0; tile < glyph_mapping_count; ++tile) {
        auto& gm = ::glyph_table->mappings_[tile];
        if (gm.valid() and gm.character_ == glyph) {
            return glyph_start_offset + tile;
        }
    }

    const auto mapping_info = mapper(glyph);

    if (not mapping_info) {
        return bad_glyph;
    }

    for (auto& info : overlay_textures) {
        if (strcmp(mapping_info->texture_name_, info.name_) == 0) {
            for (TileDesc t = 0; t < glyph_mapping_count; ++t) {
                auto& gm = ::glyph_table->mappings_[t];
                if (not gm.valid()) {
                    gm.character_ = glyph;
                    gm.reference_count_ = 0;

                    // 8 x 8 x (4 bitsperpixel / 8 bitsperbyte)
                    constexpr int tile_size = vram_tile_size();

                    // u8 buffer[tile_size] = {0};
                    // memcpy16(buffer,
                    //          (u8*)&MEM_SCREENBLOCKS[sbb_overlay_texture][0] +
                    //              ((81) * tile_size),
                    //          tile_size / 2);

                    const auto colors = font_color_indices();

                    // We need to know which color to use as the background
                    // color, and which color to use as the foreground
                    // color. Each charset needs to store a reference pixel in
                    // the top left corner, representing the background color,
                    // otherwise, we have no way of knowing which pixel color to
                    // substitute where!
                    const auto bg_color = ((u8*)info.tile_data_)[0] & 0x0f;

                    u8 buffer[tile_size] = {0};
                    memcpy16(buffer,
                             info.tile_data_ +
                                 (mapping_info->offset_ * tile_size) /
                                     sizeof(decltype(info.tile_data_)),
                             tile_size / 2);

                    for (int i = 0; i < tile_size; ++i) {
                        auto c = buffer[i];
                        if (c & bg_color) {
                            buffer[i] = colors.bg_;
                        } else {
                            buffer[i] = colors.fg_;
                        }
                        if (c & (bg_color << 4)) {
                            buffer[i] |= colors.bg_ << 4;
                        } else {
                            buffer[i] |= colors.fg_ << 4;
                        }
                    }

                    // FIXME: Why do these magic constants work? I wish better
                    // documentation existed for how the gba tile memory worked.
                    // I thought, that the tile size would be 32, because we
                    // have 4 bits per pixel, and 8x8 pixel tiles. But the
                    // actual number of bytes in a tile seems to be half of the
                    // expected number. Also, in vram, it seems like the tiles
                    // do seem to be 32 bytes apart after all...
                    memcpy16((u8*)&MEM_SCREENBLOCKS[sbb_overlay_texture][0] +
                                 ((t + glyph_start_offset) * tile_size),
                             buffer,
                             tile_size / 2);

                    return t + glyph_start_offset;
                }
            }
        }
    }
    return bad_glyph;
}


static bool is_glyph(u16 t)
{
    return t >= glyph_start_offset and
           t - glyph_start_offset < glyph_mapping_count;
}


void Platform::fill_overlay(u16 tile)
{
    if (glyph_mode and is_glyph(tile)) {
        // This is moderately complicated to implement, better off just not
        // allowing fills for character tiles.
        return;
    }

    const u16 tile_info = tile | SE_PALBANK(1);
    const u32 fill_word = tile_info | (tile_info << 16);

    u32* const mem = (u32*)overlay_back_buffer;
    overlay_back_buffer_changed = true;

    for (unsigned i = 0; i < sizeof(ScreenBlock) / sizeof(u32); ++i) {
        mem[i] = fill_word;
    }

    if (glyph_mode) {
        for (auto& gm : ::glyph_table->mappings_) {
            gm.reference_count_ = -1;
        }
    }
}


static void set_overlay_tile(Platform& pfrm, u16 x, u16 y, u16 val, int palette)
{
    if (glyph_mode) {
        // This is where we handle the reference count for mapped glyphs. If
        // we are overwriting a glyph with different tile, then we can
        // decrement a glyph's reference count. Then, we want to increment
        // the incoming glyph's reference count if the incoming tile is
        // within the range of the glyph table.

        const auto old_tile = pfrm.get_tile(Layer::overlay, x, y);
        if (old_tile not_eq val) {
            if (is_glyph(old_tile)) {
                auto& gm =
                    ::glyph_table->mappings_[old_tile - glyph_start_offset];
                if (gm.valid()) {
                    gm.reference_count_ -= 1;

                    if (gm.reference_count_ == 0) {
                        gm.reference_count_ = -1;
                        gm.character_ = 0;
                    }
                } else {
                    error(pfrm,
                          "existing tile is a glyph, but has no"
                          " mapping table entry!");
                }
            }

            if (is_glyph(val)) {
                auto& gm = ::glyph_table->mappings_[val - glyph_start_offset];
                if (not gm.valid()) {
                    // Not clear exactly what to do here... Somehow we've
                    // gotten into an erroneous state, but not a permanently
                    // unrecoverable state (tile isn't valid, so it'll be
                    // overwritten upon the next call to map_tile).
                    warning(pfrm, "invalid assignment to glyph table");
                    return;
                }
                gm.reference_count_++;
            }
        }
    }

    overlay_back_buffer[x + y * 32] = val | SE_PALBANK(palette);
    overlay_back_buffer_changed = true;
}


// Now for custom-colored text... we're using three of the background palettes
// already, one for the map_0 layer (shared with the background layer), one for
// the map_1 layer, and one for the overlay. That leaves 13 remaining palettes,
// which we can use for colored text. But we may not want to use up all of the
// available extra palettes, so let's just allocate four of them toward custom
// text colors for now...
static const PaletteBank custom_text_palette_begin = 3;
static const PaletteBank custom_text_palette_end = 7;
static const auto custom_text_palette_count =
    custom_text_palette_end - custom_text_palette_begin;

static PaletteBank custom_text_palette_write_ptr = custom_text_palette_begin;
static const TextureData* custom_text_palette_source_texture = nullptr;


void Platform::set_tile(u16 x, u16 y, TileDesc glyph, const FontColors& colors)
{
    if (not glyph_mode or not is_glyph(glyph)) {
        return;
    }

    // If the current overlay texture changed, then we'll need to clear out any
    // palettes that we've constructed. The indices of the glyph binding sites
    // in the palette bank may have changed when we loaded a new texture.
    if (custom_text_palette_source_texture and
        custom_text_palette_source_texture not_eq current_overlay_texture) {

        for (auto p = custom_text_palette_begin; p < custom_text_palette_end;
             ++p) {
            for (int i = 0; i < 16; ++i) {
                MEM_BG_PALETTE[p * 16 + i] = 0;
            }
        }

        custom_text_palette_source_texture = current_overlay_texture;
    }

    const auto default_colors = font_color_indices();

    const auto fg_color_hash = real_color(colors.foreground_).bgr_hex_555();
    const auto bg_color_hash = real_color(colors.background_).bgr_hex_555();

    auto existing_mapping = [&]() -> std::optional<PaletteBank> {
        for (auto i = custom_text_palette_begin; i < custom_text_palette_end;
             ++i) {
            if (MEM_BG_PALETTE[i * 16 + default_colors.fg_] == fg_color_hash and
                MEM_BG_PALETTE[i * 16 + default_colors.bg_] == bg_color_hash) {

                return i;
            }
        }
        return {};
    }();

    if (existing_mapping) {
        set_overlay_tile(*this, x, y, glyph, *existing_mapping);
    } else {
        const auto target = custom_text_palette_write_ptr;

        MEM_BG_PALETTE[target * 16 + default_colors.fg_] = fg_color_hash;
        MEM_BG_PALETTE[target * 16 + default_colors.bg_] = bg_color_hash;

        set_overlay_tile(*this, x, y, glyph, target);

        custom_text_palette_write_ptr =
            ((target + 1) - custom_text_palette_begin) %
                custom_text_palette_count +
            custom_text_palette_begin;

        if (custom_text_palette_write_ptr == custom_text_palette_begin) {
            warning(*this, "wraparound in custom text palette alloc");
        }
    }
}


void Platform::set_tile(Layer layer, u16 x, u16 y, u16 val)
{
    switch (layer) {
    case Layer::overlay:
        if (x > 31 or y > 31) {
            return;
        }
        set_overlay_tile(*this, x, y, val, 1);
        break;

    case Layer::map_1:
        if (x > 15 or y > 19) {
            return;
        }
        set_map_tile(sbb_t1_tiles, x, y, val, 2);
        break;

    case Layer::map_0:
        if (x > 15 or y > 19) {
            return;
        }
        set_map_tile(sbb_t0_tiles, x, y, val, 0);
        break;

    case Layer::background:
        if (x > 31 or y > 32) {
            return;
        }
        MEM_SCREENBLOCKS[sbb_bg_tiles][x + y * 32] = val;
        break;
    }
}


////////////////////////////////////////////////////////////////////////////////
// NetworkPeer
////////////////////////////////////////////////////////////////////////////////


Platform::NetworkPeer::NetworkPeer()
{
}


static int multiplayer_is_master()
{
    return (REG_SIOCNT & (1 << 2)) == 0 and (REG_SIOCNT & (1 << 3));
}


// NOTE: you may only call this function immediately after a transmission,
// otherwise, may return a garbage value.
static int multiplayer_error()
{
    return REG_SIOCNT & (1 << 6);
}


static bool multiplayer_validate_modes()
{
    // 1 if all GBAs are in the correct mode, 0 otherwise.
    return REG_SIOCNT & (1 << 3);
}


static bool multiplayer_validate()
{
    if (not multiplayer_validate_modes()) {
        return false;
    } else {
    }
    return true;
}


static int rx_message_count = 0;
static int tx_message_count = 0;
static int rx_loss = 0;
static int tx_loss = 0;


// The gameboy Multi link protocol always sends data, no matter what, even if we
// do not have any new data to put in the send buffer. Because there is no
// distinction between real data and empty transmits, we will transmit in
// fixed-size chunks. The receiver knows when it's received a whole message,
// after a specific number of iterations. Now, there are other ways, potentially
// better ways, to handle this situation. But this way seems easiest, although
// probably uses a lot of unnecessary bandwidth. Another drawback: the poller
// needs ignore messages that are all zeroes. Accomplished easily enough by
// prefixing the sent message with an enum, where the zeroth enumeration is
// unused.
static const int message_iters =
    Platform::NetworkPeer::max_message_size / sizeof(u16);

struct WireMessage {
    u16 data_[message_iters] = {};
};

using TxInfo = WireMessage;

static const int tx_ring_size = 32;

static ObjectPool<TxInfo, tx_ring_size> tx_message_pool;

static int tx_ring_write_pos = 0;
static int tx_ring_read_pos = 0;
static TxInfo* tx_ring[tx_ring_size] = {nullptr};


static TxInfo* tx_ring_pop()
{
    TxInfo* msg = nullptr;

    for (int i = tx_ring_read_pos; i < tx_ring_read_pos + tx_ring_size; ++i) {
        auto index = i % tx_ring_size;
        if (tx_ring[index]) {
            msg = tx_ring[index];
            tx_ring[index] = nullptr;
            tx_ring_read_pos = index;
            return msg;
        }
    }

    tx_ring_read_pos += 1;
    tx_ring_read_pos %= tx_ring_size;

    // The transmit ring is completely empty!
    return nullptr;
}


using RxInfo = WireMessage;

static const int rx_ring_size = 64;

static ObjectPool<RxInfo, rx_ring_size> rx_message_pool;

static int rx_ring_write_pos = 0;
static int rx_ring_read_pos = 0;
static RxInfo* rx_ring[rx_ring_size] = {nullptr};


static void rx_ring_push(RxInfo* message)
{
    rx_message_count += 1;

    if (rx_ring[rx_ring_write_pos]) {
        // The reader does not seem to be keeping up!
        rx_loss += 1;
        auto lost_message = rx_ring[rx_ring_write_pos];
        rx_ring[rx_ring_write_pos] = nullptr;
        rx_message_pool.post(lost_message);
    }

    rx_ring[rx_ring_write_pos] = message;
    rx_ring_write_pos += 1;
    rx_ring_write_pos %= rx_ring_size;
}


static RxInfo* rx_ring_pop()
{
    RxInfo* msg = nullptr;

    for (int i = rx_ring_read_pos; i < rx_ring_read_pos + rx_ring_size; ++i) {
        auto index = i % rx_ring_size;
        if (rx_ring[index]) {
            msg = rx_ring[index];
            rx_ring[index] = nullptr;
            rx_ring_read_pos = index;
            return msg;
        }
    }

    rx_ring_read_pos += 1;
    rx_ring_read_pos %= rx_ring_size;

    return nullptr;
}


static int rx_iter_state = 0;
static RxInfo* rx_current_message =
    nullptr; // Note: we will drop the first message, oh well.

static bool rx_current_all_zeroes = true; // The multi serial io mode always
                                          // transmits, even when there's
                                          // nothing to send. At first, I was
                                          // allowing zeroed out messages
                                          // generated by the platform to pass
                                          // through to the user. But doing so
                                          // takes up a lot of space in the rx
                                          // buffer, so despite the
                                          // inconvenience, for performance
                                          // reasons, I am going to have to
                                          // require that messages containing
                                          // all zeroes never be sent by the
                                          // user.


static void multiplayer_rx_receive()
{
    if (rx_iter_state == message_iters) {
        if (rx_current_message) {
            if (rx_current_all_zeroes) {
                rx_message_pool.post(rx_current_message);
            } else {
                rx_ring_push(rx_current_message);
            }
        }

        rx_current_all_zeroes = true;

        rx_current_message = rx_message_pool.get();
        if (not rx_current_message) {
            rx_loss += 1;
        }
        rx_iter_state = 0;
    }

    if (rx_current_message) {
        const auto val =
            multiplayer_is_master() ? REG_SIOMULTI1 : REG_SIOMULTI0;
        if (rx_current_all_zeroes and val) {
            rx_current_all_zeroes = false;
        }
        rx_current_message->data_[rx_iter_state++] = val;

    } else {
        rx_iter_state++;
    }
}


static int transmit_busy_count = 0;


static bool multiplayer_busy()
{
    return REG_SIOCNT & SIO_START;
}


static int tx_iter_state = 0;
static TxInfo* tx_current_message = nullptr;


static int null_bytes_written = 0;


bool Platform::NetworkPeer::send_message(const Message& message)
{
    if (message.length_ > sizeof(TxInfo::data_)) {
        while (true)
            ; // error!
    }

    if (not is_connected()) {
        return false;
    }

    // TODO: uncomment this block if we actually see issues on the real hardware...
    // if (tx_iter_state == message_iters) {
    //     // Decreases the likelihood of manipulating data shared by the interrupt
    //     // handlers. See related comment in the poll_message() function.
    //     return false;
    // }

    if (tx_ring[tx_ring_write_pos]) {
        // The writer does not seem to be keeping up! Guess we'll have to drop a
        // message :(
        tx_loss += 1;
        auto lost_message = tx_ring[tx_ring_write_pos];
        tx_ring[tx_ring_write_pos] = nullptr;
        tx_message_pool.post(lost_message);
    }

    auto msg = tx_message_pool.get();
    if (not msg) {
        // error! Could not transmit messages fast enough, i.e. we've exhausted
        // the message pool! How to handle this condition!?
        tx_loss += 1;
        return false;
    }

    __builtin_memcpy(msg->data_, message.data_, message.length_);

    tx_ring[tx_ring_write_pos] = msg;
    tx_ring_write_pos += 1;
    tx_ring_write_pos %= tx_ring_size;

    return true;
}


static void multiplayer_tx_send()
{
    if (tx_iter_state == message_iters) {
        if (tx_current_message) {
            tx_message_pool.post(tx_current_message);
            tx_message_count += 1;
        }
        tx_current_message = tx_ring_pop();
        tx_iter_state = 0;
    }

    if (tx_current_message) {
        REG_SIOMLT_SEND = tx_current_message->data_[tx_iter_state++];
    } else {
        null_bytes_written += 2;
        tx_iter_state++;
        REG_SIOMLT_SEND = 0;
    }
}


// We want to wait long enough for the minions to prepare TX data for the
// master.
static void multiplayer_schedule_master_tx()
{
    REG_TM2CNT_H = 0x00C1;
    REG_TM2CNT_L = 65000; // Be careful with this delay! Due to manufacturing
                          // differences between Gameboy Advance units, you
                          // really don't want to get too smart, and try to
                          // calculate the time right up to the boundary of
                          // where you expect the interrupt to happen. Allow
                          // some extra wiggle room, for other devices that may
                          // raise a serial receive interrupt later than you
                          // expect. Maybe, this timer could be sped up a bit,
                          // but I don't really know... here's the thing, this
                          // code CURRENTLY WORKS, so don't use a faster timer
                          // interrupt until you've tested the code on a bunch
                          // different real gba units (try out the code on the
                          // original gba, both sp models, etc.)

    irqEnable(IRQ_TIMER2);
    irqSet(IRQ_TIMER2, [] {
        if (multiplayer_busy()) {
            ++transmit_busy_count;
            return; // still busy, try again. The only thing that should kick
                    // off this timer, though, is the serial irq, and the
                    // initial connection, so not sure how we could get into
                    // this state.
        } else {
            irqDisable(IRQ_TIMER2);
            multiplayer_tx_send();
            REG_SIOCNT |= SIO_START;
        }
    });
}


static void multiplayer_schedule_tx()
{
    // If we're the minion, simply enter data into the send queue. The
    // master will wait before initiating another transmit.
    if (multiplayer_is_master()) {
        multiplayer_schedule_master_tx();
    } else {
        multiplayer_tx_send();
    }
}


static void multiplayer_serial_isr()
{
    if (UNLIKELY(multiplayer_error())) {
        ::platform->network_peer().disconnect();
        return;
    }
    multiplayer_rx_receive();
    multiplayer_schedule_tx();
}


static RxInfo* poller_current_message = nullptr;


std::optional<Platform::NetworkPeer::Message>
Platform::NetworkPeer::poll_message()
{
    if (rx_iter_state == message_iters) {
        // This further decreases the likelihood of messing up the receive
        // interrupt handler by manipulating shared data. We really should be
        // declaring stuff volatile and disabling interrupts, but we cannot
        // easily do those things, for various practical reasons, so we're just
        // hoping that a problematic interrupt during a transmit or a poll is
        // just exceedingly unlikely in practice. The serial interrupt handler
        // runs approximately twice per frame, and the game only transmits a few
        // messages per second. Furthermore, the interrupt handlers only access
        // shared state when rx_iter_state == message_iters, so only one in six
        // interrupts manipulates shared state, i.e. only one occurrence every
        // three or so frames. And for writes to shared data to even be a
        // problem, the interrupt would have to occur between two instructions
        // when writing to the message ring or to the message pool. And on top
        // of all that, we are leaving packets in the rx buffer while
        // rx_iter_state == message iters, so we really shouldn't be writing at
        // the same time anyway. So in practice, the possibility of manipulating
        // shared data is just vanishingly small, although I acknowledge that
        // it's a potential problem. There _IS_ a bug, but I've masked it pretty
        // well (I hope). No issues detectable in an emulator, but we'll see
        // about the real hardware... once my link cable arrives in the mail.
        return {};
    }
    if (auto msg = rx_ring_pop()) {
        if (UNLIKELY(poller_current_message not_eq nullptr)) {
            // failure to deallocate/consume message!
            rx_message_pool.post(msg);
            disconnect();
            return {};
        }
        poller_current_message = msg;
        return Platform::NetworkPeer::Message{
            reinterpret_cast<byte*>(msg->data_),
            static_cast<int>(sizeof(WireMessage::data_))};
    }
    return {};
}


void Platform::NetworkPeer::poll_consume(u32 size)
{
    if (poller_current_message) {
        rx_message_pool.post(poller_current_message);
    } else {
        while (true)
            ;   // How do we even end up in this scenario?! We only write
                // to poller_current_message if rx_ring_pop returns a
                // valid pointer...
    }
    poller_current_message = nullptr;
}


static bool multiplayer_connected = false;

static void multiplayer_init()
{
    ::platform->network_peer().disconnect();

    ::platform->sleep(3);

    REG_RCNT = R_MULTI;
    REG_SIOCNT = SIO_MULTI;
    REG_SIOCNT |= SIO_IRQ | SIO_115200;

    irqEnable(IRQ_SERIAL);
    irqSet(IRQ_SERIAL, multiplayer_serial_isr);

    // Put this here for now, not sure whether it's really necessary...
    REG_SIOMLT_SEND = 0x5555;

    Microseconds delta = 0;
    while (not multiplayer_validate()) {
        delta += ::platform->delta_clock().reset();
        if (delta > seconds(20)) {
            if (not multiplayer_validate_modes()) {
                error(*::platform, "not all GBAs are in MULTI mode");
            }
            ::platform->network_peer().disconnect(); // just for good measure
            REG_SIOCNT = 0;
            irqDisable(IRQ_SERIAL);
            return;
        }
        ::platform->feed_watchdog();
    }
    // When the multi link cable is actually plugged into both gameboys, the
    // connection should already be established at this point. But when no cable
    // is connected to the link port, all of the SIOCNT bits will represent a
    // valid state anyway. So let's push out a message, regardless, and wait to
    // receive a response.

    const char* handshake = "link__v0.0.1";

    if (str_len(handshake) not_eq Platform::NetworkPeer::max_message_size) {
        ::platform->network_peer().disconnect();
        error(*::platform, "handshake string does not equal message size");
        return;
    }

    multiplayer_connected = true;

    ::platform->network_peer().send_message(
        {(byte*)handshake, sizeof handshake});

    multiplayer_schedule_tx();

    while (true) {
        ::platform->feed_watchdog();
        delta += ::platform->delta_clock().reset();
        if (auto msg = ::platform->network_peer().poll_message()) {
            for (u32 i = 0; i < sizeof handshake; ++i) {
                if (((u8*)msg->data_)[i] not_eq handshake[i]) {
                    ::platform->network_peer().disconnect();
                    info(*::platform, "invalid handshake");
                    return;
                }
            }
            info(*::platform, "validated handshake");
            ::platform->network_peer().poll_consume(sizeof handshake);
            return;
        } else if (delta > seconds(20)) {
            error(*::platform,
                  "no handshake received within a reasonable window");
            ::platform->network_peer().disconnect();
            return;
        }
    }

    // OK, we've extablished a connection!
}


void Platform::NetworkPeer::connect(const char* peer)
{
    multiplayer_init();
}


void Platform::NetworkPeer::listen()
{
    multiplayer_init();
}


void Platform::NetworkPeer::update()
{
}


static int last_tx_count = 0;


Platform::NetworkPeer::Stats Platform::NetworkPeer::stats()
{
    const int empty_transmits = null_bytes_written / max_message_size;
    null_bytes_written = 0;

    Float link_saturation = 0.f;

    if (empty_transmits) {
        auto tx_diff = tx_message_count - last_tx_count;

        link_saturation = Float(tx_diff) / (empty_transmits + tx_diff);
    }

    last_tx_count = tx_message_count;

    return {tx_message_count,
            rx_message_count,
            tx_loss,
            rx_loss,
            static_cast<int>(100 * link_saturation)};
}


bool Platform::NetworkPeer::supported_by_device()
{
    return true;
}


bool Platform::NetworkPeer::is_connected() const
{
    return multiplayer_connected; // multiplayer_validate(); // FIXME: insufficient to detect disconnects.
}


bool Platform::NetworkPeer::is_host() const
{
    return multiplayer_is_master();
}


void Platform::NetworkPeer::disconnect()
{
    // Be very careful editing this function. We need to get ourselves back to a
    // completely clean slate, otherwise, we won't be able to reconnect (e.g. if
    // you leave a message sitting in the transmit ring, it may be erroneously
    // sent out when you try to reconnect, instead of the handshake message);
    if (is_connected()) {
        info(*::platform, "disconnected!");
        multiplayer_connected = false;
        irqDisable(IRQ_SERIAL);
        if (multiplayer_is_master()) {
            enable_watchdog();
        }
        REG_SIOCNT = 0;

        if (poller_current_message) {
            // Not sure whether this is the correct thing to do here...
            rx_message_pool.post(poller_current_message);
            poller_current_message = nullptr;
        }

        rx_iter_state = 0;
        if (rx_current_message) {
            rx_message_pool.post(rx_current_message);
            rx_current_message = nullptr;
        }
        rx_current_all_zeroes = true;
        for (auto& msg : rx_ring) {
            if (msg) {
                rx_message_pool.post(msg);
                msg = nullptr;
            }
        }
        rx_ring_write_pos = 0;
        rx_ring_read_pos = 0;

        tx_iter_state = 0;
        if (tx_current_message) {
            tx_message_pool.post(tx_current_message);
            tx_current_message = nullptr;
        }
        for (auto& msg : tx_ring) {
            if (msg) {
                tx_message_pool.post(msg);
                msg = nullptr;
            }
        }
        tx_ring_write_pos = 0;
        tx_ring_read_pos = 0;
    }
}


Platform::NetworkPeer::Interface Platform::NetworkPeer::interface() const
{
    return Interface::serial_cable;
}


Platform::NetworkPeer::~NetworkPeer()
{
    // ...
}


////////////////////////////////////////////////////////////////////////////////
// SystemClock
//
// Uses the cartridge RTC hardware, over the gpio port.
//
////////////////////////////////////////////////////////////////////////////////


static void rtc_gpio_write_command(u8 value)
{
    u8 temp;

    for (u8 i = 0; i < 8; i++) {
        temp = ((value >> (7 - i)) & 1);
        S3511A_GPIO_PORT_DATA = (temp << 1) | 4;
        S3511A_GPIO_PORT_DATA = (temp << 1) | 4;
        S3511A_GPIO_PORT_DATA = (temp << 1) | 4;
        S3511A_GPIO_PORT_DATA = (temp << 1) | 5;
    }
}


[[gnu::
      unused]] // Currently unused, but this is how you would write to the chip...
static void
rtc_gpio_write_data(u8 value)
{
    u8 temp;

    for (u8 i = 0; i < 8; i++) {
        temp = ((value >> i) & 1);
        S3511A_GPIO_PORT_DATA = (temp << 1) | 4;
        S3511A_GPIO_PORT_DATA = (temp << 1) | 4;
        S3511A_GPIO_PORT_DATA = (temp << 1) | 4;
        S3511A_GPIO_PORT_DATA = (temp << 1) | 5;
    }
}


static u8 rtc_gpio_read_value()
{
    u8 temp;
    u8 value = 0;

    for (u8 i = 0; i < 8; i++) {
        S3511A_GPIO_PORT_DATA = 4;
        S3511A_GPIO_PORT_DATA = 4;
        S3511A_GPIO_PORT_DATA = 4;
        S3511A_GPIO_PORT_DATA = 4;
        S3511A_GPIO_PORT_DATA = 4;
        S3511A_GPIO_PORT_DATA = 5;

        temp = ((S3511A_GPIO_PORT_DATA & 2) >> 1);
        value = (value >> 1) | (temp << 7);
    }

    return value;
}


static u8 rtc_get_status()
{
    S3511A_GPIO_PORT_DATA = 1;
    S3511A_GPIO_PORT_DATA = 5;
    S3511A_GPIO_PORT_DIRECTION = 7;

    rtc_gpio_write_command(S3511A_CMD_STATUS | S3511A_RD);

    S3511A_GPIO_PORT_DIRECTION = 5;

    const auto status = rtc_gpio_read_value();

    S3511A_GPIO_PORT_DATA = 1;
    S3511A_GPIO_PORT_DATA = 1;

    return status;
}


static auto rtc_get_datetime()
{
    std::array<u8, 7> result;

    S3511A_GPIO_PORT_DATA = 1;
    S3511A_GPIO_PORT_DATA = 5;
    S3511A_GPIO_PORT_DIRECTION = 7;

    rtc_gpio_write_command(S3511A_CMD_DATETIME | S3511A_RD);

    S3511A_GPIO_PORT_DIRECTION = 5;

    for (auto& val : result) {
        val = rtc_gpio_read_value();
    }

    result[4] &= 0x7F;

    S3511A_GPIO_PORT_DATA = 1;
    S3511A_GPIO_PORT_DATA = 1;

    return result;
}


Platform::SystemClock::SystemClock()
{
}


static u32 bcd_to_binary(u8 bcd)
{
    if (bcd > 0x9f)
        return 0xff;

    if ((bcd & 0xf) <= 9)
        return (10 * ((bcd >> 4) & 0xf)) + (bcd & 0xf);
    else
        return 0xff;
}


std::optional<DateTime> Platform::SystemClock::now()
{
    if (rtc_faulty) {
        return {};
    }

    REG_IME = 0; // hopefully we don't miss anything important, like a serial
                 // interrupt! But nothing should call SystemClock::now() very
                 // often...

    const auto [year, month, day, dow, hr, min, sec] = rtc_get_datetime();

    REG_IME = 1;

    DateTime info;
    info.date_.year_ = bcd_to_binary(year);
    info.date_.month_ = bcd_to_binary(month);
    info.date_.day_ = bcd_to_binary(day);
    info.hour_ = bcd_to_binary(hr);
    info.minute_ = bcd_to_binary(min);
    info.second_ = bcd_to_binary(sec);

    return info;
}


void Platform::SystemClock::init(Platform& pfrm)
{
    S3511A_GPIO_PORT_READ_ENABLE = 1;

    auto status = rtc_get_status();
    if (status & S3511A_STATUS_POWER) {
        error(pfrm, "RTC chip power failure");
    }
}


#endif // __GBA__
