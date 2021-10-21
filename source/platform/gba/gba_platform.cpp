
////////////////////////////////////////////////////////////////////////////////
//
//
// Platform Implementation for Nintendo Gameboy Advance
//
//
////////////////////////////////////////////////////////////////////////////////

#ifdef __GBA__

#include "bulkAllocator.hpp"
#include "gba_color.hpp"
#include "gbp_logo.hpp"
#include "graphics/overlay.hpp"
#include "localization.hpp"
#include "number/random.hpp"
#include "platform/platform.hpp"
#include "rumble.h"
#include "script/lisp.hpp"
#include "string.hpp"
#include "util.hpp"
#include <algorithm>


void english__to_string(int num, char* buffer, int base);


#include "gba.h"


namespace {

class RemoteConsoleLispPrinter : public lisp::Printer {
public:
    RemoteConsoleLispPrinter(Platform& pfrm) : pfrm_(pfrm)
    {
    }

    void put_str(const char* str) override
    {
        fmt_ += str;
    }

    Platform::RemoteConsole::Line fmt_;
    Platform& pfrm_;
};

} // namespace


static int overlay_y = 0;


enum class GlobalFlag {
    rtc_faulty,
    gbp_unlocked,
    multiplayer_connected,
    night_mode,
    save_using_flash,
    glyph_mode,
    count
};


static Bitvector<static_cast<int>(GlobalFlag::count)> gflags;


static void set_gflag(GlobalFlag f, bool val)
{
    gflags.set(static_cast<int>(f), val);
}


static bool get_gflag(GlobalFlag f)
{
    return gflags.get(static_cast<int>(f));
}


struct BiosVersion {
    enum {
        NDS = static_cast<long unsigned int>(-1162995584),
        GBA = static_cast<long unsigned int>(-1162995585)
    };
};


Platform::DeviceName Platform::device_name() const
{
    switch (BiosCheckSum()) {
    case BiosVersion::NDS:
        return "NintendoDS";

    case BiosVersion::GBA:
        return "GameboyAdvance";

    default:
        return "Unknown";
    }
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


static constexpr int vram_tile_size()
{
    // 8 x 8 x (4 bitsperpixel / 8 bitsperbyte)
    return 32;
}


#include "images.cpp"


static const TextureData* current_spritesheet = &sprite_textures[0];
static const TextureData* current_tilesheet0 = &tile_textures[0];
static const TextureData* current_tilesheet1 = &tile_textures[1];
static const TextureData* current_overlay_texture = &overlay_textures[1];


void start(Platform&);


static Platform* platform;


// Thanks to Windows, main is technically platform specific (WinMain)
int main(int argc, char** argv)
{
    gflags.clear();

    Platform pf;
    ::platform = &pf;

    start(pf);
}


const char* Platform::get_opt(char opt)
{
    // Command line arguments aren't supported, seeing we are running without
    // an operating system.
    return nullptr;
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
    // P.S.: Now, I've discovered that the screen refresh rate is actually 59.73
    // Hz. Sorry to have created a headache for anyone in the future who may be
    // attempting to port this game.
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


std::optional<Bitvector<int(Key::count)>> missed_keys;


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

    if (UNLIKELY(static_cast<bool>(::missed_keys))) {
        for (int i = 0; i < (int)Key::count; ++i) {
            if ((*::missed_keys)[i]) {
                states_[i] = true;
            }
        }
        ::missed_keys.reset();
    }
}


void Platform::Keyboard::rumble(bool enabled)
{
    // We have a working RTC chip connected to the cartridge gpio, so do not
    // attempt to write rumble commands.
    if (not get_gflag(GlobalFlag::rtc_faulty)) {
        return;
    }

    if (enabled) {
        rumble_set_state(RumbleState::rumble_start);
    } else {
        rumble_set_state(RumbleState::rumble_stop);
    }
}


////////////////////////////////////////////////////////////////////////////////
// Screen
////////////////////////////////////////////////////////////////////////////////


struct alignas(4) ObjectAttributes {
    u16 attribute_0;
    u16 attribute_1;
    u16 attribute_2;

    s16 affine_transform;
};


// See documentation. Object memory provides thirty-two matrices for affine
// transformation; the parameters nestled between every four objects.
struct alignas(4) ObjectAffineMatrix {
    ObjectAttributes o0;
    ObjectAttributes o1;
    ObjectAttributes o2;
    ObjectAttributes o3;

    auto& pa()
    {
        return o0.affine_transform;
    }
    auto& pb()
    {
        return o1.affine_transform;
    }
    auto& pc()
    {
        return o2.affine_transform;
    }
    auto& pd()
    {
        return o3.affine_transform;
    }

    void identity()
    {
        pa() = 0x0100l;
        pb() = 0;
        pc() = 0;
        pd() = 0x0100;
    }

    void scale(s16 sx, s16 sy)
    {
        pa() = (1 << 8) - sx;
        pb() = 0;
        pc() = 0;
        pd() = (1 << 8) - sy;
    }

    void rotate(s16 degrees)
    {
        // I have no recollection of why the shift by seven works. I saw some
        // libraries shift by four, but that seemed not to work from what I
        // remember. Everyone seems to use a different sine lookup table, might
        // be the culprit.
        const int ss = sine(degrees) >> 7;
        const int cc = cosine(degrees) >> 7;

        pa() = cc;
        pb() = -ss;
        pc() = ss;
        pd() = cc;
    }

    void rot_scale(s16 degrees, s16 x, s16 y)
    {
        // FIXME: This code doesn't seem to work correctly yet...
        const int ss = sine(degrees);
        const int cc = cosine(degrees);

        pa() = cc * x >> 12;
        pb() = -ss * x >> 12;
        pc() = ss * y >> 12;
        pd() = cc * y >> 12;
    }
};


namespace attr0_mask {
constexpr u16 disabled{2 << 8};
}


constexpr u32 oam_count = Platform::Screen::sprite_limit;


static ObjectAttributes* const object_attribute_memory = {
    (ObjectAttributes*)0x07000000};

static ObjectAttributes
    object_attribute_back_buffer[Platform::Screen::sprite_limit];


static auto affine_transform_back_buffer =
    reinterpret_cast<ObjectAffineMatrix*>(object_attribute_back_buffer);


static const u32 affine_transform_limit = 32;
static u32 affine_transform_write_index = 0;
static u32 last_affine_transform_write_index = 0;


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


// Most of the game uses tile-based graphics modes, but some parts of the intro
// sequence, which display the gameboy player logo, currently use the bitmap
// graphics modes, for simplicity.
static void init_video(Platform::Screen& screen)
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

    View view;
    view.set_size(screen.size().cast<Float>());

    screen.set_view(view);

    REG_MOSAIC = MOS_BUILD(0, 0, 1, 1);
}


static bool unlock_gameboy_player(Platform& pfrm)
{
    bool gbp_detected = false;

    RegisterRamReset(RESET_VRAM);


    REG_DISPCNT = MODE_0 | BG0_ENABLE;
    *bg0_control = 0x0088;
    *bg0_x_scroll = 0;
    *bg0_y_scroll = 0;

    static const auto white_555 = Color(custom_color(0xffffff)).bgr_hex_555();

    // The fade effect is costly, pre-populate the palette with the start color
    // to mitigate (visible) tearing.
    for (u32 i = 0; i < sizeof(gbp_logo_palette) / 2; ++i) {
        MEM_BG_PALETTE[i] = white_555;
    }

    auto push_palette = [&](const Color& to, u8 blend_amount) {
        if (blend_amount == 0) {
            memcpy16(MEM_BG_PALETTE,
                     gbp_logo_palette,
                     (sizeof gbp_logo_palette) / 2);
            return;
        }
        static const int palette_count = (sizeof gbp_logo_palette) / 2;
        for (int i = 0; i < palette_count; ++i) {
            const auto c = Color::from_bgr_hex_555(((u16*)gbp_logo_palette)[i]);
            MEM_BG_PALETTE[i] = blend(c, to, blend_amount);
        }
    };

    // Show the Gameboy Player splash screen.
    memcpy16((u16*)0x6008000, gbp_logo_pixels, (sizeof gbp_logo_pixels) / 2);
    memcpy16((u16*)0x6000000, gbp_logo_tiles, (sizeof gbp_logo_tiles) / 2);

    static const int fadein_frames = 40;
    static const int fadeout_frames = 25;

    // The fadein/out effects are not required for gbp unlocking. Just looks
    // clean.
    for (int i = 0; i < fadein_frames; ++i) {
        push_palette(custom_color(0xffffff),
                     (1 - Float(i) / fadein_frames) * 255);
        // The gameboy player will probably not be unlocked during the fade, but
        // you never know, I guess...
        if (*keys == 0x030F) {
            gbp_detected = true;
        }

        VBlankIntrWait();
    }

    push_palette(ColorConstant::rich_black, 0);

    using Frames = int;
    static const Frames splashscreen_duration(125);

    for (Frames i = 0; i < splashscreen_duration; ++i) {
        // If the gameboy player hardware/software accepted our spash screen,
        // the system will raise a joystick state where L+R+U+D on the D-Pad are
        // all pressed at once. If we see this keymask, we know that we have
        // successfully unlocked the gameboy player.
        if (*keys == 0x030F) {
            gbp_detected = true;
        }

        VBlankIntrWait();
    }

    for (int i = 0; i < fadeout_frames; ++i) {
        push_palette(custom_color(0xffffff), (Float(i) / fadeout_frames) * 255);
        VBlankIntrWait();
    }

    RegisterRamReset(RESET_VRAM);

    return gbp_detected;
}


Platform::Screen::Screen() : userdata_(nullptr)
{
}


static u32 last_oam_write_index = 0;
static u32 oam_write_index = 0;


Color real_color(ColorConstant k)
{
    switch (k) {
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

    case ColorConstant::aged_paper:
        static const Color aged_paper(27, 24, 18);
        return aged_paper;

    case ColorConstant::silver_white:
        static const Color silver_white(29, 29, 30);
        return silver_white;

    case ColorConstant::rich_black:
        static const Color rich_black(0, 0, 2);
        return rich_black;

    default:
        return Color(k);
    }
}


using PaletteBank = int;
constexpr PaletteBank available_palettes = 3;
constexpr PaletteBank palette_count = 16;

static PaletteBank palette_counter = available_palettes;


static u8 screen_pixelate_amount = 0;


Color adjust_warmth(const Color& c, int amount)
{
    auto ret = c;
    ret.r_ = clamp(c.r_ + amount, 0, 31);
    ret.b_ = clamp(c.b_ - amount, 0, 31);

    return ret;
}


static Color nightmode_adjust(const Color& c)
{
    if (not get_gflag(GlobalFlag::night_mode)) {
        return c;
    } else {
        return adjust_warmth(
            Color::from_bgr_hex_555(blend(c, c.grayscale(), 190)), 2);
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
    bool used_ = false;
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
            info.used_ = true;
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
        // Ok, so in this instance, we ran out of unused palettes, but there may
        // be a palette that was used in the last frame, and which has not yet
        // been referenced while rendering the current frame. Unlock that
        // palette and use it. Note that doing this could cause sprites to
        // flicker, because there's still a sprite in the OAM front buffer using
        // the locked-but-not-used palette bank. We would not need two booleans,
        // locked_ and used_, in the table for keeping state if we simply used a
        // palette back buffer, but adding a back buffer for the OAM palettes
        // complicates the logic in other ways, uses more memory, and would
        // require us to copy the whole sprite palette bank into VRAM on every
        // frame. Because we typically aren't exhausting our supply of ColorMix
        // palettes anyway, we don't do double buffering for sprite palettes,
        // and instead, consciously avoid writing to palette banks used by
        // sprites in the previous frame, unless we really run out of palettes
        // (which is a rare case, so not worth the cost of double buffering,
        // IMO).
        if (auto salvaged_palette = [&] {
                for (PaletteBank palette = available_palettes; palette < 16;
                     ++palette) {
                    auto& info = palette_info[palette];
                    if (info.locked_ and not info.used_) {
                        return palette;
                    }
                }
                return 0;
            }()) {
            palette_counter = salvaged_palette;
        } else {
            return 0;
        }
    }

    const auto c = nightmode_adjust(real_color(k));

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

    palette_info[palette_counter] = {k, amount, true, true};

    return palette_counter++;
}


static struct DynamicTextureMapping {
    bool reserved_ = false;
    bool dirty_ = false;
    u16 spritesheet_offset_ = 0;
} dynamic_texture_mappings[Platform::dynamic_texture_count];


u16 find_dynamic_mapping(u16 virtual_index)
{
    // FIXME: this code will not work for 32x32 pixel sprites yet!
    for (int i = 0; i < Platform::dynamic_texture_count; ++i) {
        if (virtual_index == dynamic_texture_mappings[i].spritesheet_offset_ and
            dynamic_texture_mappings[i].reserved_) {
            return i * 2;
        }
    }
    warning(*::platform, "mapping not found");
    return 0;
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
        const auto position =
            spr.get_position().cast<s32>() - spr.get_origin().cast<s32>();

        const auto view_center = view_.get_center().cast<s32>();
        auto oa = object_attribute_back_buffer + oam_write_index;
        if (spr.get_alpha() not_eq Sprite::Alpha::translucent) {
            oa->attribute_0 = ATTR0_COLOR_16 | ATTR0_TALL;
        } else {
            oa->attribute_0 = ATTR0_COLOR_16 | ATTR0_TALL | ATTR0_BLEND;
        }
        oa->attribute_1 = ATTR1_SIZE_32; // clear attr1

        auto abs_position = position - view_center;

        oa->attribute_0 &= (0xff00 & ~((1 << 8) | (1 << 9))); // clear attr0

        if (spr.get_rotation() or spr.get_scale().x or spr.get_scale().y) {
            if (affine_transform_write_index not_eq affine_transform_limit) {
                auto& affine =
                    affine_transform_back_buffer[affine_transform_write_index];

                if (spr.get_rotation() and
                    (spr.get_scale().x or spr.get_scale().y)) {
                    affine.rot_scale(spr.get_rotation(),
                                     spr.get_scale().x,
                                     spr.get_scale().y);
                } else if (spr.get_rotation()) {
                    affine.rotate(spr.get_rotation());
                } else {
                    affine.scale(spr.get_scale().x, spr.get_scale().y);
                }

                oa->attribute_0 |= 1 << 8;
                oa->attribute_0 |= 1 << 9;

                abs_position.x -= 8;
                abs_position.y -= 16;

                oa->attribute_1 |= affine_transform_write_index << 9;

                affine_transform_write_index += 1;
            }
        } else {
            const auto& flip = spr.get_flip();
            oa->attribute_1 |= ((int)flip.y << 13);
            oa->attribute_1 |= ((int)flip.x << 12);
        }

        oa->attribute_0 |= abs_position.y & 0x00ff;

        if ((mix.amount_ > 215 and mix.amount_ < 255) or
            screen_pixelate_amount not_eq 0) {

            oa->attribute_0 |= ATTR0_MOSAIC;
        }

        oa->attribute_1 |= (abs_position.x + x_off) & 0x01ff;

        const auto w16_h32_index =
            spr.get_texture_index() * (scale == 16 ? 2 : 1);
        auto ti = spr.get_texture_index();
        if (w16_h32_index > 125) {
            ti = find_dynamic_mapping(w16_h32_index);
            if (scale == 16) {
                ti /= 2;
            }
        }
        const auto target_index = 2 + ti * scale + tex_off;
        oa->attribute_2 = target_index;
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


static Buffer<Platform::Task*, 7> task_queue_;


void Platform::push_task(Task* task)
{
    task->complete_ = false;
    task->running_ = true;

    if (not task_queue_.push_back(task)) {
        fatal("failed to enqueue task");
    }
}


static void map_dynamic_textures()
{
    for (int i = 0; i < Platform::dynamic_texture_count; ++i) {
        auto& mapping = dynamic_texture_mappings[i];
        if (mapping.dirty_) {
            // Ok, so now, we want to perform a copy from ROM into the reserved
            // VRAM for our dynamic texture.
            const auto offset = mapping.spritesheet_offset_;
            const u8* image_data = (const u8*)current_spritesheet->tile_data_;
            u8* spr_vram_base_addr = (u8*)&MEM_TILE[4][1];

            constexpr auto single_tile_size = vram_tile_size();
            constexpr auto w32_h16_chunk_size = single_tile_size * 8;
            constexpr auto w32_h32_chunk_size = w32_h16_chunk_size * 2;

            memcpy32(spr_vram_base_addr + w32_h16_chunk_size * (i * 2),
                     image_data + w32_h16_chunk_size * offset,
                     // NOTE: we always copy in 32x32 chunks. Our dynamic tiles
                     // need to support either 32x16 or 32x32 chunks, and we do
                     // not know enough about how the sprites are going to
                     // leverage the video memory to know whether the remapped
                     // tiles will be drawn with 32x16 or 32x32 pixel sprites.
                     w32_h32_chunk_size / 4);

            mapping.dirty_ = false;
        }
    }
}


void Platform::Screen::clear()
{
    rumble_update();

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


    // We want to do the dynamic texture remapping near the screen clear, to
    // reduce tearing. Most of the other changes that we make to vram, like the
    // overlay tiles and OAM, are double-buffered, so the tearing is less
    // noticable if we perform the copies further from the site of the vsync.
    map_dynamic_textures();

    auto view_offset = view_.get_center().cast<s32>();
    *bg0_x_scroll = view_offset.x;
    *bg0_y_scroll = view_offset.y;

    *bg3_x_scroll = view_offset.x;
    *bg3_y_scroll = view_offset.y;
}


[[noreturn]] static void restart()
{
    // NOTE: I am clearing almost everything, because when I did not clear all
    // of these memory segments, something seemed to be interfering with gameboy
    // player unlocking.
    RegisterRamReset(RESET_VRAM | RESET_PALETTE | RESET_OAM | RESET_SIO |
                     RESET_SOUND | RESET_OTHER);
    SoftReset(ROM_RESTART), __builtin_unreachable();
}


static void keypad_isr()
{
    restart();
}


static ScreenBlock overlay_back_buffer alignas(u32);
static bool overlay_back_buffer_changed = false;


void Platform::Screen::display()
{
    // platform->stopwatch().start();

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
        // Disable affine transform for unused sprite
        object_attribute_back_buffer[i].attribute_0 &= ~((1 << 8) | (1 << 9));
        object_attribute_back_buffer[i].attribute_1 = 0;

        object_attribute_back_buffer[i].attribute_0 |= attr0_mask::disabled;
    }

    for (u32 i = affine_transform_write_index;
         i < last_affine_transform_write_index;
         ++i) {

        auto& affine = affine_transform_back_buffer[i];
        affine.pa() = 0;
        affine.pb() = 0;
        affine.pc() = 0;
        affine.pd() = 0;
    }

    // I noticed less graphical artifacts when using a back buffer. I thought I
    // would see better performance when writing directly to OAM, rather than
    // doing a copy later, but I did not notice any performance difference when
    // adding a back buffer.
    memcpy32(object_attribute_memory,
             object_attribute_back_buffer,
             (sizeof object_attribute_back_buffer) / 4);

    last_affine_transform_write_index = affine_transform_write_index;
    affine_transform_write_index = 0;

    last_oam_write_index = oam_write_index;
    oam_write_index = 0;
    palette_counter = available_palettes;

    for (auto& info : palette_info) {
        if (not info.used_) {
            info.locked_ = false;
        }
        info.used_ = false;
    }

    auto view_offset = view_.get_center().cast<s32>();

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


static u16 sprite_palette[16];
static u16 tilesheet_0_palette[16];
static u16 tilesheet_1_palette[16];
static u16 overlay_palette[16];


// We use base_contrast as the starting point for all contrast calculations. In
// most screen modes, the base contrast will be zero, but in some situations,
// like when we have night mode enabled, the base contrast will be decreased,
// and then further contrast adjustments will be calculated according to the
// shifted base value.
static Contrast base_contrast = 0;
static Contrast contrast = 0;


Contrast Platform::Screen::get_contrast() const
{
    return ::contrast;
}


static void
init_palette(const TextureData* td, u16* palette, bool skip_contrast)
{
    const auto adj_cr = contrast + base_contrast;

    for (int i = 0; i < 16; ++i) {
        if (not skip_contrast and adj_cr not_eq 0) {
            const Float f = (259.f * (adj_cr + 255)) / (255 * (259 - adj_cr));
            const auto c =
                nightmode_adjust(Color::from_bgr_hex_555(td->palette_data_[i]));

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
            palette[i] =
                nightmode_adjust(Color::from_bgr_hex_555(td->palette_data_[i]))
                    .bgr_hex_555();
        }
    }
}


void Platform::Screen::enable_night_mode(bool enabled)
{
    set_gflag(GlobalFlag::night_mode, enabled);

    if (enabled) {
        ::base_contrast = -12;
    } else {
        ::base_contrast = 0;
    }

    init_palette(current_spritesheet, sprite_palette, false);
    init_palette(current_tilesheet0, tilesheet_0_palette, false);
    init_palette(current_tilesheet1, tilesheet_1_palette, false);
    init_palette(current_overlay_texture, overlay_palette, true);

    // TODO: Edit code so that we don't need a specific hack here for the
    // overlay palette.
    for (int i = 0; i < 16; ++i) {
        MEM_BG_PALETTE[16 + i] = overlay_palette[i];
    }
}


void Platform::Screen::set_contrast(Contrast c)
{
    ::contrast = c;

    init_palette(current_spritesheet, sprite_palette, false);
    init_palette(current_tilesheet0, tilesheet_0_palette, false);
    init_palette(current_tilesheet1, tilesheet_1_palette, false);
    init_palette(current_overlay_texture, overlay_palette, true);
}


static bool validate_tilemap_texture_size(Platform& pfrm, size_t size)
{
    constexpr auto charblock_size = sizeof(ScreenBlock) * 7;
    if (size > charblock_size) {
        pfrm.fatal("tileset exceeds charblock size");
        return false;
    }
    return true;
}


static bool validate_overlay_texture_size(Platform& pfrm, size_t size)
{
    constexpr auto charblock_size = sizeof(ScreenBlock) * 8;
    if (size > charblock_size) {
        pfrm.fatal("tileset exceeds charblock size");
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


void Platform::fatal(const char* msg)
{
    const auto bkg_color = custom_color(0xcb1500);

    irqDisable(IRQ_TIMER2 | IRQ_KEYPAD | IRQ_TIMER3 | IRQ_VBLANK);


    ::platform->screen().fade(1.f, bkg_color);
    ::platform->fill_overlay(0);
    ::platform->load_overlay_texture("overlay");
    enable_glyph_mode(true);

    static const Text::OptColors text_colors{
        {custom_color(0xffffff), bkg_color}};

    static const Text::OptColors text_colors_inv{
        {text_colors->background_, text_colors->foreground_}};

    Text text(*::platform, {1, 1});
    text.append("fatal error:", text_colors_inv);

    Text text2(*::platform, {1, 3});
    text2.append(msg, text_colors);

    Buffer<Text, 6> line_buffer;
    int offset = 0;

    auto render_line = [&](const char* line,
                           int spacing,
                           Text::OptColors colors) {
        line_buffer.emplace_back(*::platform, OverlayCoord{1, u8(6 + offset)});
        line_buffer.back().append(line, colors);
        offset += spacing;
    };

    render_line("uart console available", 2, text_colors);
    render_line("link port        rs232 cable", 2, text_colors_inv);
    render_line("  SO  ---------------> RxD", 2, text_colors);
    render_line("  SI  ---------------> TxD", 2, text_colors);
    render_line("  GND ---------------> GND", 2, text_colors);
    render_line("    3.3 volts, 9600 baud    ", 2, text_colors_inv);

    Text text3(*::platform, {1, 18});
    text3.append("press start to reset...", text_colors);


    ::platform->screen().display();

    while (true) {

        if (auto line = ::platform->remote_console().readline()) {
            RemoteConsoleLispPrinter printer(*::platform);

            lisp::read(line->c_str());
            lisp::eval(lisp::get_op(0));
            format(lisp::get_op(0), printer);

            lisp::pop_op();
            lisp::pop_op();

            platform->remote_console().printline(printer.fmt_.c_str());
        }

        ::platform->keyboard().poll();
        if (::platform->keyboard().pressed<Key::start>()) {
            restart();
        }
    }
}


void Platform::set_overlay_origin(Float x, Float y)
{
    *bg2_x_scroll = static_cast<s16>(x);
    *bg2_y_scroll = static_cast<s16>(y);
    overlay_y = y;
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

    const auto c = nightmode_adjust(real_color(k));

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
                auto from = Color::from_bgr_hex_555(overlay_palette[i]);
                MEM_BG_PALETTE[16 + i] =
                    blend(from, c, include_overlay ? amt : 0);
            }
        }
        overlay_was_faded = include_overlay;
    } else {
        const auto bc = nightmode_adjust(real_color(*base));
        for (int i = 0; i < 16; ++i) {
            MEM_PALETTE[i] = blend(bc, c, include_sprites ? amt : 0);
            MEM_BG_PALETTE[i] = blend(bc, c, amt);
            MEM_BG_PALETTE[32 + i] = blend(bc, c, amt);

            if (overlay_was_faded) {
                // FIXME!
                for (int i = 0; i < 16; ++i) {
                    auto from = Color::from_bgr_hex_555(overlay_palette[i]);
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


static ObjectPool<RcBase<Platform::DynamicTexture,
                         Platform::dynamic_texture_count>::ControlBlock,
                  Platform::dynamic_texture_count>
    dynamic_texture_pool;


void Platform::DynamicTexture::remap(u16 spritesheet_offset)
{
    auto& mapping = dynamic_texture_mappings[mapping_index_];
    mapping.dirty_ = true;
    mapping.spritesheet_offset_ = spritesheet_offset;
}


std::optional<Platform::DynamicTexturePtr> Platform::make_dynamic_texture()
{
    auto finalizer =
        [](RcBase<Platform::DynamicTexture,
                  Platform::dynamic_texture_count>::ControlBlock* ctrl) {
            dynamic_texture_mappings[ctrl->data_.mapping_index()].reserved_ =
                false;
            dynamic_texture_pool.post(ctrl);
        };

    for (u8 i = 0; i < dynamic_texture_count; ++i) {
        if (not dynamic_texture_mappings[i].reserved_) {
            auto dt =
                DynamicTexturePtr::create(&dynamic_texture_pool, finalizer, i);
            if (dt) {
                dynamic_texture_mappings[i].reserved_ = true;
                return *dt;
            }
        }
    }

    warning(*this, "Failed to allocate DynamicTexture.");
    return {};
}


void Platform::load_sprite_texture(const char* name)
{
    for (auto& mapping : dynamic_texture_mappings) {
        mapping.dirty_ = true;
    }

    for (auto& info : sprite_textures) {

        if (str_cmp(name, info.name_) == 0) {

            current_spritesheet = &info;

            init_palette(current_spritesheet, sprite_palette, false);


            // NOTE: There are four tile blocks, so index four points to the
            // end of the tile memory.
            memcpy16((void*)&MEM_TILE[4][1],
                     info.tile_data_,
                     std::min((u32)16128, info.tile_data_length_ / 2));

            // We need to do this, otherwise whatever screen fade is currently
            // active will be overwritten by the copy.
            const auto c = nightmode_adjust(real_color(last_color));
            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(sprite_palette[i]);
                MEM_PALETTE[i] = blend(from, c, last_fade_amt);
            }
        }
    }

    // By replacing the sprite texture, we overwrote whatever was in the dynamic
    // texture memory. Let's try an recover.
    map_dynamic_textures();
}


void Platform::load_tile0_texture(const char* name)
{
    for (auto& info : tile_textures) {

        if (str_cmp(name, info.name_) == 0) {

            current_tilesheet0 = &info;

            init_palette(current_tilesheet0, tilesheet_0_palette, false);


            // We don't want to load the whole palette into memory, we might
            // overwrite palettes used by someone else, e.g. the overlay...
            //
            // Also, like the sprite texture, we need to apply the currently
            // active screen fade while modifying the color palette.
            const auto c = nightmode_adjust(real_color(last_color));
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

        if (str_cmp(name, info.name_) == 0) {

            current_tilesheet1 = &info;

            init_palette(current_tilesheet1, tilesheet_1_palette, false);


            // We don't want to load the whole palette into memory, we might
            // overwrite palettes used by someone else, e.g. the overlay...
            //
            // Also, like the sprite texture, we need to apply the currently
            // active screen fade while modifying the color palette.
            const auto c = nightmode_adjust(real_color(last_color));
            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(tilesheet_1_palette[i]);
                MEM_BG_PALETTE[32 + i] = blend(from, c, last_fade_amt);
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

    // NOTE: When sleeping for large numbers of frames, we may miss button
    // presses! So we should keep track of which keys were pressed during the
    // sleep() call.
    Keyboard temp_kb;

    const auto start_keys = keyboard_.dump_state();

    while (frames--) {
        temp_kb.poll();
        const auto current_keys = temp_kb.dump_state();

        for (int i = 0; i < (int)Key::count; ++i) {
            if (start_keys[i] not_eq current_keys[i] and current_keys[i]) {
                if (not ::missed_keys) {
                    ::missed_keys.emplace();
                    ::missed_keys->clear();
                }
                missed_keys->set(i, true);
            }
        }

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


// NOTE: Some cartridge manufacturers back in the day searched ROMS for a
// word-aligned string, to determine what type of save memory to put on the
// chip. I designed the code to use either SRAM or FLASH, but let's include the
// backup ID string anyway, because we'd really prefer to have SRAM. Unlikely
// that anyone would ever agree to make me a GBA cartridge, but hey, you never
// know...
READ_ONLY_DATA alignas(4) [[gnu::used]] static const
    char backup_type[] = {'S', 'R', 'A', 'M', '_', 'V', 'n', 'n', 'n'};


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


bool Platform::write_save_data(const void* data, u32 length, u32 offset)
{
    if (get_gflag(GlobalFlag::save_using_flash)) {
        return flash_save(data, offset, length);
    } else {
        sram_save(data, offset, length);
        return true;
    }
}


bool Platform::read_save_data(void* buffer, u32 data_length, u32 offset)
{
    if (get_gflag(GlobalFlag::save_using_flash)) {
        flash_load(buffer, offset, data_length);
    } else {
        sram_load(buffer, offset, data_length);
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


static const u32 initial_log_write_loc = 16000;
static u32 log_write_loc = initial_log_write_loc;


Platform::Logger::Logger()
{
}


static Severity log_threshold;


void Platform::Logger::set_threshold(Severity severity)
{
    log_threshold = severity;
}


static void mgba_log(const char* msg)
{
    *reinterpret_cast<uint16_t*>(0x4FFF780) = 0xC0DE;

    int max_characters_per_line = 256;
    auto reg_debug_string = reinterpret_cast<char*>(0x4FFF600);
    int characters_left = str_len(msg);

    while (characters_left > 0) {
        volatile u16& reg_debug_flags = *reinterpret_cast<u16*>(0x4FFF700);

        int characters_to_write =
            std::min(characters_left, max_characters_per_line);
        __builtin_memcpy(reg_debug_string, msg, characters_to_write);
        reg_debug_flags = 2 | 0x100;
        msg += characters_to_write;
        characters_left -= characters_to_write;
    }
}


void Platform::Logger::log(Severity level, const char* msg)
{
    // if (static_cast<int>(level) < static_cast<int>(::log_threshold)) {
    //     return;
    // }

    // We don't want to wear out the flash chip! The code below still works on
    // flash though, if you just comment out the if statement below.
    if (get_gflag(GlobalFlag::save_using_flash)) {
        return;
    }

    std::array<char, 256> buffer;

    buffer[1] = ':';

    switch (level) {
    case Severity::debug:
        buffer[0] = 'd';
        break;

    case Severity::info:
        buffer[0] = 'i';
        break;

    case Severity::warning:
        buffer[0] = 'w';
        break;

    case Severity::error:
        buffer[0] = 'E';
        break;

    case Severity::fatal:
        buffer[0] = 'f';
        break;

    case Severity::count:
        return;
    }

    const auto msg_size = str_len(msg);

    u32 i;
    constexpr size_t prefix_size = 2;

    for (i = 0;
         i < std::min(size_t(msg_size), buffer.size() - (prefix_size + 1));
         ++i) {
        buffer[i + prefix_size] = msg[i];
    }
    buffer[i + prefix_size] = '\n';
    buffer[i + prefix_size + 1] =
        '\n'; // This char will be overwritten, meant to identify
              // the end of the log, in the case where the log wraps
              // around.

    if (log_write_loc + prefix_size + msg_size + 2 >= 64000) {
        // Out of loggin space! We could wrap around to the beginning...
        // log_write_loc = initial_log_write_loc;
    } else {
        if (get_gflag(GlobalFlag::save_using_flash)) {
            flash_save(buffer.data(), log_write_loc, buffer.size());
        } else {
            sram_save(buffer.data(), log_write_loc, buffer.size());
        }
    }

    log_write_loc += msg_size + prefix_size + 1;

    mgba_log(buffer.data());
}


void Platform::Logger::read(void* buffer, u32 start_offset, u32 num_bytes)
{
    if (get_gflag(GlobalFlag::save_using_flash)) {
        flash_load(buffer, initial_log_write_loc + start_offset, num_bytes);
    } else {
        sram_load(buffer, initial_log_write_loc + start_offset, num_bytes);
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
#include "data/music_chantiers_navals_412.hpp"
#include "data/music_midsommar.hpp"
#include "data/music_murmuration.hpp"
#include "data/music_rocketlaunch.hpp"
#include "data/music_waves.hpp"
#include "data/scottbuckley_computations.hpp"
#include "data/scottbuckley_omega.hpp"


static const int null_music_len = 8;
static const u32 null_music[null_music_len] = {0, 0, 0, 0, 0, 0, 0, 0};


#define DEF_AUDIO(__STR_NAME__, __TRACK_NAME__, __DIV__)                       \
    {                                                                          \
        STR(__STR_NAME__), (AudioSample*)__TRACK_NAME__,                       \
            __TRACK_NAME__##Len / __DIV__                                      \
    }


#define DEF_MUSIC(__STR_NAME__, __TRACK_NAME__)                                \
    DEF_AUDIO(__STR_NAME__, __TRACK_NAME__, 4)


#define DEF_SOUND(__STR_NAME__, __TRACK_NAME__)                                \
    DEF_AUDIO(__STR_NAME__, __TRACK_NAME__, 1)


#include "gba_platform_soundcontext.hpp"


SoundContext snd_ctx;


static const struct AudioTrack {
    const char* name_;
    const AudioSample* data_;
    int length_; // NOTE: For music, this is the track length in 32 bit words,
                 // but for sounds, length_ reprepresents bytes.
} music_tracks[] = {DEF_MUSIC(omega, scottbuckley_omega),
                    DEF_MUSIC(computations, scottbuckley_computations),
                    DEF_MUSIC(clair_de_lune, clair_de_lune),
                    DEF_MUSIC(murmuration, music_murmuration),
                    DEF_MUSIC(rocketlaunch, music_rocketlaunch),
                    DEF_MUSIC(chantiers_navals_412, music_chantiers_navals_412),
                    DEF_MUSIC(midsommar, music_midsommar),
                    DEF_MUSIC(waves, music_waves)};


static const AudioTrack* find_music(const char* name)
{
    for (auto& track : music_tracks) {

        if (str_cmp(name, track.name_) == 0) {
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
#include "data/sound_dodge.hpp"
#include "data/sound_dropitem.hpp"
#include "data/sound_explosion1.hpp"
#include "data/sound_explosion2.hpp"
#include "data/sound_footstep1.hpp"
#include "data/sound_footstep2.hpp"
#include "data/sound_footstep3.hpp"
#include "data/sound_footstep4.hpp"
#include "data/sound_heart.hpp"
#include "data/sound_laser1.hpp"
#include "data/sound_msg.hpp"
#include "data/sound_open_book.hpp"
#include "data/sound_openbag.hpp"
#include "data/sound_pop.hpp"
#include "data/sound_scroll.hpp"
#include "data/sound_select.hpp"
#include "data/sound_thud.hpp"
#include "data/sound_tw_bell.hpp"
#include "data/sound_typewriter.hpp"


static const AudioTrack sounds[] = {DEF_SOUND(explosion1, sound_explosion1),
                                    DEF_SOUND(explosion2, sound_explosion2),
                                    DEF_SOUND(typewriter, sound_typewriter),
                                    DEF_SOUND(footstep1, sound_footstep1),
                                    DEF_SOUND(footstep2, sound_footstep2),
                                    DEF_SOUND(footstep3, sound_footstep3),
                                    DEF_SOUND(footstep4, sound_footstep4),
                                    DEF_SOUND(open_book, sound_open_book),
                                    DEF_SOUND(dropitem, sound_dropitem),
                                    DEF_SOUND(openbag, sound_openbag),
                                    DEF_SOUND(blaster, sound_blaster),
                                    DEF_SOUND(tw_bell, sound_tw_bell),
                                    DEF_SOUND(select, sound_select),
                                    DEF_SOUND(laser1, sound_laser1),
                                    DEF_SOUND(scroll, sound_scroll),
                                    DEF_SOUND(creak, sound_creak),
                                    DEF_SOUND(dodge, sound_dodge),
                                    DEF_SOUND(heart, sound_heart),
                                    DEF_SOUND(click, sound_click),
                                    DEF_SOUND(thud, sound_thud),
                                    DEF_SOUND(coin, sound_coin),
                                    DEF_SOUND(bell, sound_bell),
                                    DEF_SOUND(pop, sound_pop),
                                    DEF_SOUND(msg, sound_msg)};


static const AudioTrack* get_sound(const char* name)
{
    for (auto& sound : sounds) {
        if (str_cmp(name, sound.name_) == 0) {
            return &sound;
        }
    }
    return nullptr;
}


Microseconds Platform::Speaker::track_length(const char* name)
{
    if (const auto music = find_music(name)) {
        return (music->length_ * sizeof(u32)) / 0.016f;
    }

    if (const auto sound = get_sound(name)) {
        return sound->length_ / 0.016f;
    }

    return 0;
}


namespace detail {
template <std::size_t... Is> struct seq {
};
template <std::size_t N, std::size_t... Is>
struct gen_seq : gen_seq<N - 1, N - 1, Is...> {
};
template <std::size_t... Is> struct gen_seq<0, Is...> : seq<Is...> {
};


template <class Generator, std::size_t... Is>
constexpr auto generate_array_helper(Generator g, seq<Is...>)
    -> std::array<decltype(g(std::size_t{}, sizeof...(Is))), sizeof...(Is)>
{
    return {{g(Is, sizeof...(Is))...}};
}

template <std::size_t tcount, class Generator>
constexpr auto generate_array(Generator g)
    -> decltype(generate_array_helper(g, gen_seq<tcount>{}))
{
    return generate_array_helper(g, gen_seq<tcount>{});
}
} // namespace detail


auto make_volume_lut = [](float scale) {
    return detail::generate_array<256>(
        [scale](std::size_t curr, std::size_t total) -> s8 {
            const auto real = (s8)((u8)curr);
            return real * scale;
        });
};


// Each table entry contains the whole number space of a signed 8-bit value,
// scaled by a fraction.
static constexpr std::array<VolumeScaleLUT, 20> volume_scale_LUTs = {
    {{make_volume_lut(0.05f)}, {make_volume_lut(0.10f)},
     {make_volume_lut(0.15f)}, {make_volume_lut(0.20f)},
     {make_volume_lut(0.25f)}, {make_volume_lut(0.30f)},
     {make_volume_lut(0.35f)}, {make_volume_lut(0.40f)},
     {make_volume_lut(0.45f)}, {make_volume_lut(0.50f)},
     {make_volume_lut(0.55f)}, {make_volume_lut(0.60f)},
     {make_volume_lut(0.65f)}, {make_volume_lut(0.70f)},
     {make_volume_lut(0.75f)}, {make_volume_lut(0.80f)},
     {make_volume_lut(0.85f)}, {make_volume_lut(0.90f)},
     {make_volume_lut(0.95f)}, {make_volume_lut(1.0f)}},
};


static const VolumeScaleLUT* get_volume_lut(Float volume)
{
    return &volume_scale_LUTs[volume * (volume_scale_LUTs.size() - 1)];
}


static void
set_sound_volume(ActiveSoundInfo& sound, Float left_volume, Float right_volume)
{
    sound.l_volume_lut_ = get_volume_lut(left_volume);
    sound.r_volume_lut_ = get_volume_lut(right_volume);
}


static std::optional<ActiveSoundInfo> make_sound(const char* name)
{
    if (auto sound = get_sound(name)) {
        return ActiveSoundInfo{0,
                               sound->length_,
                               sound->data_,
                               0,
                               &volume_scale_LUTs[19],
                               &volume_scale_LUTs[19]};
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


bool Platform::Speaker::is_music_playing(const char* name)
{
    bool playing = false;

    if (auto track = find_music(name)) {
        modify_audio([&] {
            if (track->data_ == snd_ctx.music_track) {
                playing = true;
            }
        });
    }

    return playing;
}


static Vec2<Float> spatialized_audio_listener_pos;


void Platform::Speaker::set_position(const Vec2<Float>& position)
{
    spatialized_audio_listener_pos = position;
}


static void add_sound(Buffer<ActiveSoundInfo, 3>& sounds,
                      const ActiveSoundInfo& info)
{
    if (not sounds.full()) {
        sounds.push_back(info);
    } else {
        ActiveSoundInfo* lowest = sounds.begin();
        for (auto it = sounds.begin(); it not_eq sounds.end(); ++it) {
            if (it->priority_ < lowest->priority_) {
                lowest = it;
            }
        }

        if (lowest not_eq sounds.end() and lowest->priority_ < info.priority_) {
            sounds.erase(lowest);
            sounds.push_back(info);
        }
    }
}


void Platform::Speaker::play_sound(const char* name,
                                   int priority,
                                   std::optional<Vec2<Float>> position)
{
    (void)position; // We're not using position data, because on the gameboy
                    // advance, we aren't supporting spatial audio.

    if (auto info = make_sound(name)) {
        info->priority_ = priority;

        if (position) {
            const auto dist =
                distance(*position, spatialized_audio_listener_pos);

            if (dist < 48) {
                set_sound_volume(*info, 1.f, 1.f);
            } else {
                const Float distance_scale = 0.0005f;

                const auto inv_sqr_intensity =
                    1.f / (distance_scale * ((dist / 4) * dist));

                auto l_vol = inv_sqr_intensity;
                auto r_vol = inv_sqr_intensity;

                // Currently disabled, used for stereo sound effects
                // const auto dir =
                //     direction(spatialized_audio_listener_pos, *position);
                // const auto attenuation = 1.f - inv_sqr_intensity;
                // const auto partial_attenuation = attenuation / 2.f;

                // l_vol -= partial_attenuation * dir.x;
                // r_vol += partial_attenuation * dir.x;

                // if (r_vol < 0 and l_vol > r_vol) {
                //     l_vol += r_vol;
                // }

                // if (l_vol < 0 and l_vol > r_vol) {
                //     r_vol += l_vol;
                // }

                l_vol = clamp(l_vol, 0.f, 1.f);
                r_vol = clamp(r_vol, 0.f, 1.f);

                set_sound_volume(*info, l_vol, r_vol);
            }

        } else {
            set_sound_volume(*info, 1.f, 1.f);
        }

        modify_audio([&] { add_sound(snd_ctx.active_sounds, *info); });
    }
}


static void clear_music()
{
    // The audio interrupt handler can be smaller and simpler if we use a track
    // of empty bytes to represent scenarios where music is not playing, rather
    // than adding another if condition to the audio isr.
    snd_ctx.music_track = reinterpret_cast<const AudioSample*>(null_music);
    snd_ctx.music_track_length = null_music_len - 1;
    snd_ctx.music_track_pos = 0;
}


static void stop_music()
{
    modify_audio([] { clear_music(); });
}


void Platform::Speaker::stop_music()
{
    ::stop_music();
}


static void play_music(const char* name, Microseconds offset)
{
    const auto track = find_music(name);
    if (track == nullptr) {
        return;
    }

    const Microseconds sample_offset = offset * 0.016f; // NOTE: because 16kHz

    modify_audio([&] {
        snd_ctx.music_track_length = track->length_;
        snd_ctx.music_track = track->data_;
        snd_ctx.music_track_pos = (sample_offset / 4) % track->length_;
    });
}


void Platform::Speaker::play_music(const char* name, Microseconds offset)
{
    // NOTE: The sound sample needs to be mono, and 8-bit signed. To export this
    // format from Audacity, convert the tracks to mono via the Tracks dropdown,
    // and then export as raw, in the format 8-bit signed.
    //
    // Also, important to convert the sound file frequency to 16kHz.

    this->stop_music();

    ::play_music(name, offset);

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
#define REG_SGFIFOB *(volatile u32*)0x40000A4


// NOTE: I tried to move this audio update interrupt handler to IWRAM, but the
// sound output became sort of glitchy, and I noticed some tearing in the
// display. At the same time, the game was less laggy, so maybe when I work out
// the kinks, this function will eventually be moved to arm code instead of
// thumb.
//
// NOTE: We play music at 16kHz, and we can load four samples upon each audio
// interrupt, i.e. 4000 interrupts per second, i.e. approximately sixty-seven
// interrupts per frame (given sixty fps). Considering how many interrupts we're
// dealing with here, this isr should be kept small and simple. We're only
// supporting one music channel (which loops by default), and three concurrent
// sound channels, in our audio mixer.
//
// Considering the number of interrupts that we're dealing with here, one might
// wonder why we aren't using one of the DMA channels to load sound samples. The
// DMA halts the CPU, which could result in missed serial I/O interrupts during
// multiplayer games.
[[maybe_unused]] static void audio_update_spatialized_stereo_isr()
{
    alignas(4) AudioSample mixing_buffer_l[4];
    alignas(4) AudioSample mixing_buffer_r[4];

    // NOTE: audio tracks in ROM should therefore have four byte alignment!
    *((u32*)mixing_buffer_l) =
        ((u32*)(snd_ctx.music_track))[snd_ctx.music_track_pos++];

    *((u32*)mixing_buffer_r) = *((u32*)mixing_buffer_l);

    if (UNLIKELY(snd_ctx.music_track_pos > snd_ctx.music_track_length)) {
        snd_ctx.music_track_pos = 0;
    }

    for (auto it = snd_ctx.active_sounds.begin();
         it not_eq snd_ctx.active_sounds.end();) {
        if (UNLIKELY(it->position_ + 4 >= it->length_)) {
            it = snd_ctx.active_sounds.erase(it);
        } else {
            for (int i = 0; i < 4; ++i) {
                mixing_buffer_r[i] +=
                    (*it->r_volume_lut_)[(u8)it->data_[it->position_]];
                mixing_buffer_l[i] +=
                    (*it->l_volume_lut_)[(u8)it->data_[it->position_]];
                ++it->position_;
            }
            ++it;
        }
    }

    REG_SGFIFOA = *((u32*)mixing_buffer_r);
    REG_SGFIFOB = *((u32*)mixing_buffer_l);
}


[[maybe_unused]] static void audio_update_spatialized_isr()
{
    alignas(4) AudioSample mixing_buffer[4];

    // NOTE: audio tracks in ROM should therefore have four byte alignment!
    *((u32*)mixing_buffer) =
        ((u32*)(snd_ctx.music_track))[snd_ctx.music_track_pos++];

    if (UNLIKELY(snd_ctx.music_track_pos > snd_ctx.music_track_length)) {
        snd_ctx.music_track_pos = 0;
    }

    for (auto it = snd_ctx.active_sounds.begin();
         it not_eq snd_ctx.active_sounds.end();) {
        if (UNLIKELY(it->position_ + 4 >= it->length_)) {
            it = snd_ctx.active_sounds.erase(it);
        } else {
            for (int i = 0; i < 4; ++i) {
                mixing_buffer[i] +=
                    (*it->r_volume_lut_)[(u8)it->data_[it->position_]];
                ++it->position_;
            }
            ++it;
        }
    }

    REG_SGFIFOA = *((u32*)mixing_buffer);
    // REG_SGFIFOB = *((u32*)mixing_buffer);
}


// Simpler mixer, without stereo sound or volume modulation, for multiplayer
// games.
static void audio_update_fast_isr()
{
    alignas(4) AudioSample mixing_buffer[4];

    // NOTE: audio tracks in ROM should therefore have four byte alignment!
    *((u32*)mixing_buffer) =
        ((u32*)(snd_ctx.music_track))[snd_ctx.music_track_pos++];

    if (UNLIKELY(snd_ctx.music_track_pos > snd_ctx.music_track_length)) {
        snd_ctx.music_track_pos = 0;
    }

    for (auto it = snd_ctx.active_sounds.begin();
         it not_eq snd_ctx.active_sounds.end();) {
        if (UNLIKELY(it->position_ + 4 >= it->length_)) {
            it = snd_ctx.active_sounds.erase(it);
        } else {
            for (int i = 0; i < 4; ++i) {
                mixing_buffer[i] += (u8)it->data_[it->position_];
                ++it->position_;
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

    if (UNLIKELY(::watchdog_counter > seconds(10))) {
        ::watchdog_counter = 0;

        ::platform->speaker().stop_music();

        if (::platform and ::watchdog_callback) {
            (*::watchdog_callback)(*platform);
        }

        // this seems not to work :(
        // unrecoverrable_error("game froze, watchdog not fed");
        restart();
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


#include "files.cpp"


const char* Platform::load_file_contents(const char* folder,
                                         const char* filename) const
{
    for (auto& file : files) {
        if (str_cmp(file.name_, filename) == 0 and
            str_cmp(file.root_, folder) == 0) {
            return reinterpret_cast<const char*>(file.data_);
        }
    }
    return nullptr;
}


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
    ObjectPool<RcBase<ScratchBuffer, scratch_buffer_count>::ControlBlock,
               scratch_buffer_count>
        scratch_buffer_pool;


static int scratch_buffers_in_use = 0;
static int scratch_buffer_highwater = 0;


ScratchBufferPtr Platform::make_scratch_buffer()
{
    auto finalizer =
        [](RcBase<ScratchBuffer, scratch_buffer_count>::ControlBlock* ctrl) {
            --scratch_buffers_in_use;
            ctrl->pool_->post(ctrl);
        };

    auto maybe_buffer = Rc<ScratchBuffer, scratch_buffer_count>::create(
        &scratch_buffer_pool, finalizer);
    if (maybe_buffer) {
        ++scratch_buffers_in_use;
        if (scratch_buffers_in_use > scratch_buffer_highwater) {
            scratch_buffer_highwater = scratch_buffers_in_use;

            StringBuffer<60> str = "sbr highwater: ";

            str += to_string<10>(scratch_buffer_highwater).c_str();

            info(*::platform, str.c_str());
        }
        return *maybe_buffer;
    } else {
        screen().fade(1.f, ColorConstant::electric_blue);
        fatal("scratch buffer pool exhausted");
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
    u16 mapper_offset_;

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


static constexpr const auto glyph_expanded_count = 160;


static int glyph_table_size = glyph_mapping_count;


static const int font_color_index_tile = 81;


struct GlyphTable {
    GlyphMapping mappings_[glyph_mapping_count + glyph_expanded_count];
};

static std::optional<DynamicMemory<GlyphTable>> glyph_table;


void Platform::enable_expanded_glyph_mode(bool enabled)
{
    for (auto& gm : ::glyph_table->obj_->mappings_) {
        gm.reference_count_ = -1;
    }

    if (enabled) {
        glyph_table_size = glyph_mapping_count + glyph_expanded_count;
    } else {
        glyph_table_size = glyph_mapping_count;
    }
}


static void audio_start()
{
    clear_music();

    REG_SOUNDCNT_H =
        0x0B0F; //DirectSound A + fifo reset + max volume to L and R
    REG_SOUNDCNT_X = 0x0080; //turn sound chip on


    // Required for stereo, currently unused.
    // // Both direct sound channels, FIFO reset, A is R, B is L.
    // REG_SOUNDCNT_H = 0b1010100100001111;
    // REG_SOUNDCNT_X = 0x0080; //turn sound chip on


    irqEnable(IRQ_TIMER1);
    irqSet(IRQ_TIMER1, audio_update_fast_isr);

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
    return false;
    Microseconds counter = 0;

    const auto tm1 = pfrm.system_clock().now();

    while (counter < seconds(1) + milliseconds(250)) {
        counter += pfrm.delta_clock().reset();
    }

    const auto tm2 = pfrm.system_clock().now();

    return tm1 and tm2 and time_diff(*tm1, *tm2) > 0;
}


static std::optional<DateTime> start_time;


std::optional<DateTime> Platform::startup_time() const
{
    return ::start_time;
}


static void start_remote_console(Platform& pfrm);


extern char __iwram_start__;
extern char __data_end__;
extern char __ewram_start;
extern char __eheap_start;


Platform::Platform()
{
    logger().set_threshold(Severity::fatal);

    keyboard().poll();
    if (keyboard().pressed<Key::alt_1>() and
        keyboard().pressed<Key::alt_1>() and
        keyboard().pressed<Key::action_1>()) {
        logger().set_threshold(Severity::debug);
    }

    // Not sure how else to determine whether the cartridge has sram, flash, or
    // something else. An sram write will fail if the cartridge ram is flash, so
    // attempt to save, and if the save fails, assume flash. I don't really know
    // anything about the EEPROM hardware interface...
    static const int sram_test_const = 0xAAAAAAAA;
    sram_save(&sram_test_const, log_write_loc, sizeof sram_test_const);

    int sram_test_result = 0;
    sram_load(&sram_test_result, log_write_loc, sizeof sram_test_result);

    if (sram_test_result not_eq sram_test_const) {
        set_gflag(GlobalFlag::save_using_flash, true);
        info(*this, "SRAM write failed, falling back to FLASH");
    }

    glyph_table.emplace(allocate_dynamic<GlyphTable>(*this));
    if (not glyph_table) {
        error(*this, "failed to allocate glyph table");
        restart();
    }

    {
        StringBuffer<32> used("iwram used: ");
        used += to_string<10>(&__data_end__ - &__iwram_start__);
        info(*this, used.c_str());

        used = "ewram used: ";
        used += to_string<10>(&__eheap_start - &__ewram_start);
        info(*this, used.c_str());
    }

    // IMPORTANT: No calls to map_glyph() are allowed before reaching this
    // line. Otherwise, the glyph table has not yet been constructed.

    info(*this, "Verifying BIOS...");

    switch (BiosCheckSum()) {
    case BiosVersion::NDS:
        info(*this, "BIOS matches Nintendo DS");
        break;
    case BiosVersion::GBA:
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


    irqInit(); // NOTE: Do not move these lines with respect to
               // unlock_gameboy_player(), or you could break the rumble
               // unlocking.
    irqEnable(IRQ_VBLANK);

    if (unlock_gameboy_player(*this)) {
        info(*this, "gameboy player unlocked!");

        set_gflag(GlobalFlag::gbp_unlocked, true);

        RumbleGBPConfig conf{[](void (*rumble_isr)(void)) {
            irqEnable(IRQ_SERIAL);
            irqSet(IRQ_SERIAL, rumble_isr);
        }};

        rumble_init(&conf);

    } else {
        info(*this, "gbp not detected");

        start_remote_console(*this);

        rumble_init(nullptr);
    }

    irqSet(IRQ_KEYPAD, keypad_isr);
    irqEnable(IRQ_KEYPAD);
    REG_KEYCNT =
        KEY_SELECT | KEY_START | KEY_R | KEY_L | KEYIRQ_ENABLE | KEYIRQ_AND;

    init_video(screen());

    irqSet(IRQ_TIMER3, [] {
        delta_total += 0xffff;

        REG_TM3CNT_H = 0;
        REG_TM3CNT_L = 0;
        REG_TM3CNT_H = 1 << 7 | 1 << 6;
    });

    if (not rtc_verify_operability(*this)) {
        set_gflag(GlobalFlag::rtc_faulty, true);
        info(*this, "RTC chip appears either non-existant or non-functional");
    } else {
        ::start_time = system_clock_.now();

        StringBuffer<100> str = "startup time: ";

        log_format_time(str, *::start_time);

        info(*::platform, str.c_str());
    }

    enable_watchdog();

    audio_start();

    fill_overlay(0);

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


void Platform::enable_glyph_mode(bool enabled)
{
    if (enabled) {
        for (auto& gm : ::glyph_table->obj_->mappings_) {
            gm.reference_count_ = -1;
        }
    }
    set_gflag(GlobalFlag::glyph_mode, enabled);
}


static u8* overlay_vram_tile_data(u16 tile_index)
{
    return (u8*)&MEM_SCREENBLOCKS[sbb_overlay_texture][0] +
           ((tile_index)*vram_tile_size());
}


// Clever pirates will ultimately find ways of removing the text, but we can
// still make things a bit more difficult. At least we can prevent clueless
// unsophisticated pirates from selling the game.
//
// The translator for the chinese edition of the game told me that unless there
// is some in-game text indicating that the game is freeware and not meant for
// commercial use, it will be pirated and sold. I'm not too concerned about
// people selling the game actually, it doesn't bother me, but I want to protect
// ordinary folks who may be tricked into buying a free game.
[[maybe_unused]] static int chinese_noncommercial_text_checksum()
{
    int checksum = 0;

    int last = 0;

    for (int i = 368; i < 373; ++i) {
        u8* t = overlay_vram_tile_data(i);

        for (int j = 0; j < vram_tile_size(); ++j) {
            checksum += t[j];
        }
        int result = checksum ^ last;

        last = checksum;

        checksum = result;
    }

    return checksum;
}


static volatile int chinese_checksum_1 = 900;
static volatile int chinese_checksum_2 = 30;
static volatile int chinese_checksum_3 = 8;


bool Platform::overlay_texture_exists(const char* name)
{
    for (auto& info : overlay_textures) {

        if (str_cmp(name, info.name_) == 0) {
            return true;
        }
    }

    return false;
}


bool Platform::load_overlay_texture(const char* name)
{
    for (auto& info : overlay_textures) {

        if (str_cmp(name, info.name_) == 0) {

            current_overlay_texture = &info;

            init_palette(current_overlay_texture, overlay_palette, true);

            for (int i = 0; i < 16; ++i) {
                auto from = Color::from_bgr_hex_555(overlay_palette[i]);
                if (not overlay_was_faded) {
                    MEM_BG_PALETTE[16 + i] = from.bgr_hex_555();
                } else {
                    const auto c = nightmode_adjust(real_color(last_color));
                    MEM_BG_PALETTE[16 + i] = blend(from, c, last_fade_amt);
                }
            }

            if (validate_overlay_texture_size(*this, info.tile_data_length_)) {
                memcpy16((void*)&MEM_SCREENBLOCKS[sbb_overlay_texture][0],
                         info.tile_data_,
                         info.tile_data_length_ / 2);
            }

            if (get_gflag(GlobalFlag::glyph_mode)) {
                for (auto& gm : ::glyph_table->obj_->mappings_) {
                    gm.reference_count_ = -1;
                }
            }

            if (str_cmp(name, "overlay") == 0) {
                int checksum = chinese_noncommercial_text_checksum();

                if (checksum not_eq chinese_checksum_1 + chinese_checksum_2 +
                                        chinese_checksum_3) {
                    while (true) {
                    }
                }
            }

            return true;
        }
    }

    return false;
}


static const TileDesc bad_glyph = 495;


// Rather than doing tons of extra work to keep the palettes
// coordinated between different image files, use tile index
// 81 as a registration block, which holds a set of colors
// to use when mapping glyphs into vram.
static u8* font_index_tile()
{
    return (u8*)&MEM_SCREENBLOCKS[sbb_overlay_texture][0] +
           ((font_color_index_tile)*vram_tile_size());
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
                             const TextureMapping& mapping_info)
{
    if (not get_gflag(GlobalFlag::glyph_mode)) {
        return bad_glyph;
    }

    // const auto mapping_info = mapper(glyph);

    // if (not mapping_info) {
    //     return bad_glyph;
    // }

    for (TileDesc tile = 0; tile < glyph_table_size; ++tile) {
        auto& gm = ::glyph_table->obj_->mappings_[tile];
        if (gm.valid() and gm.mapper_offset_ == mapping_info.offset_) {
            return glyph_start_offset + tile;
        }
    }

    for (auto& info : overlay_textures) {
        if (str_cmp(mapping_info.texture_name_, info.name_) == 0) {
            for (TileDesc t = 0; t < glyph_table_size; ++t) {

                if (t == font_color_index_tile - 1) {
                    // When I originally created the text mapping engine, I did
                    // not expect to need to deal with languages with more than
                    // 80 distinct font tiles onscreen at a time. So, I thought
                    // it would be fine to put a metadata tile in index 81. But
                    // while working on the Chinese localization, I discovered
                    // that 80 tiles would not be nearly sufficient to display a
                    // fullscreen block of chinese words. So I needed to build a
                    // dynamically-expandable glyph table, which, when needed,
                    // can expand to consume more of the available vram. So, we
                    // need to skip over this metadata tile, to make sure that
                    // we don't overwrite it when using a larger glyph array.
                    continue;
                }

                auto& gm = ::glyph_table->obj_->mappings_[t];
                if (not gm.valid()) {
                    gm.mapper_offset_ = mapping_info.offset_;
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
                                 ((u32)mapping_info.offset_ * tile_size) /
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
           t - glyph_start_offset < glyph_table_size;
}


void Platform::fill_overlay(u16 tile)
{
    if (get_gflag(GlobalFlag::glyph_mode) and is_glyph(tile)) {
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

    if (get_gflag(GlobalFlag::glyph_mode)) {
        for (auto& gm : ::glyph_table->obj_->mappings_) {
            gm.reference_count_ = -1;
        }
    }
}


static void set_overlay_tile(Platform& pfrm, u16 x, u16 y, u16 val, int palette)
{
    if (get_gflag(GlobalFlag::glyph_mode)) {
        // This is where we handle the reference count for mapped glyphs. If
        // we are overwriting a glyph with different tile, then we can
        // decrement a glyph's reference count. Then, we want to increment
        // the incoming glyph's reference count if the incoming tile is
        // within the range of the glyph table.

        const auto old_tile = pfrm.get_tile(Layer::overlay, x, y);
        if (old_tile not_eq val) {
            if (is_glyph(old_tile)) {
                auto& gm = ::glyph_table->obj_
                               ->mappings_[old_tile - glyph_start_offset];
                if (gm.valid()) {
                    gm.reference_count_ -= 1;

                    if (gm.reference_count_ == 0) {
                        gm.reference_count_ = -1;
                        gm.mapper_offset_ = 0;
                    }
                } else {
                    error(pfrm,
                          "existing tile is a glyph, but has no"
                          " mapping table entry!");
                }
            }

            if (is_glyph(val)) {
                auto& gm =
                    ::glyph_table->obj_->mappings_[val - glyph_start_offset];
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
    if (not get_gflag(GlobalFlag::glyph_mode) or not is_glyph(glyph)) {
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

    const auto fg_color_hash =
        nightmode_adjust(real_color(colors.foreground_)).bgr_hex_555();

    const auto bg_color_hash =
        nightmode_adjust(real_color(colors.background_)).bgr_hex_555();

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


// NOTE: This function should only be called in a serial interrupt handler,
// otherwise, may return a garbage value.


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
using RxInfo = WireMessage;


struct MultiplayerComms {
    int rx_loss = 0;
    int tx_loss = 0;

    int rx_message_count = 0;
    int tx_message_count = 0;


    static constexpr const int tx_ring_size = 32;
    ObjectPool<TxInfo, tx_ring_size> tx_message_pool;

    int tx_ring_write_pos = 0;
    int tx_ring_read_pos = 0;
    TxInfo* tx_ring[tx_ring_size] = {nullptr};


    static constexpr const int rx_ring_size = 64;
    ObjectPool<RxInfo, rx_ring_size> rx_message_pool;

    int rx_ring_write_pos = 0;
    int rx_ring_read_pos = 0;
    RxInfo* rx_ring[rx_ring_size] = {nullptr};

    int rx_iter_state = 0;
    RxInfo* rx_current_message =
        nullptr; // Note: we will drop the first message, oh well.

    // The multi serial io mode always transmits, even when there's nothing to
    // send. At first, I was allowing zeroed out messages generated by the
    // platform to pass through to the user. But doing so takes up a lot of
    // space in the rx buffer, so despite the inconvenience, for performance
    // reasons, I am going to have to require that messages containing all
    // zeroes never be sent by the user.
    bool rx_current_all_zeroes = true;

    int transmit_busy_count = 0;


    int tx_iter_state = 0;
    TxInfo* tx_current_message = nullptr;

    int null_bytes_written = 0;

    bool is_host = false;

    RxInfo* poller_current_message = nullptr;
};


static MultiplayerComms multiplayer_comms;


static TxInfo* tx_ring_pop()
{
    auto& mc = multiplayer_comms;


    TxInfo* msg = nullptr;

    for (int i = mc.tx_ring_read_pos; i < mc.tx_ring_read_pos + mc.tx_ring_size;
         ++i) {
        auto index = i % mc.tx_ring_size;
        if (mc.tx_ring[index]) {
            msg = mc.tx_ring[index];
            mc.tx_ring[index] = nullptr;
            mc.tx_ring_read_pos = index;
            return msg;
        }
    }

    mc.tx_ring_read_pos += 1;
    mc.tx_ring_read_pos %= mc.tx_ring_size;

    // The transmit ring is completely empty!
    return nullptr;
}


static void rx_ring_push(RxInfo* message)
{
    auto& mc = multiplayer_comms;

    mc.rx_message_count += 1;

    if (mc.rx_ring[mc.rx_ring_write_pos]) {
        // The reader does not seem to be keeping up!
        mc.rx_loss += 1;

        auto lost_message = mc.rx_ring[mc.rx_ring_write_pos];

        mc.rx_ring[mc.rx_ring_write_pos] = nullptr;
        mc.rx_message_pool.post(lost_message);
    }

    mc.rx_ring[mc.rx_ring_write_pos] = message;
    mc.rx_ring_write_pos += 1;
    mc.rx_ring_write_pos %= mc.rx_ring_size;
}


static RxInfo* rx_ring_pop()
{
    auto& mc = multiplayer_comms;

    RxInfo* msg = nullptr;

    for (int i = mc.rx_ring_read_pos; i < mc.rx_ring_read_pos + mc.rx_ring_size;
         ++i) {
        auto index = i % mc.rx_ring_size;

        if (mc.rx_ring[index]) {
            msg = mc.rx_ring[index];
            mc.rx_ring[index] = nullptr;
            mc.rx_ring_read_pos = index;

            return msg;
        }
    }

    mc.rx_ring_read_pos += 1;
    mc.rx_ring_read_pos %= mc.rx_ring_size;

    return nullptr;
}


static void multiplayer_rx_receive()
{
    auto& mc = multiplayer_comms;

    if (mc.rx_iter_state == message_iters) {
        if (mc.rx_current_message) {
            if (mc.rx_current_all_zeroes) {
                mc.rx_message_pool.post(mc.rx_current_message);
            } else {
                rx_ring_push(mc.rx_current_message);
            }
        }

        mc.rx_current_all_zeroes = true;

        mc.rx_current_message = mc.rx_message_pool.get();
        if (not mc.rx_current_message) {
            mc.rx_loss += 1;
        }
        mc.rx_iter_state = 0;
    }

    if (mc.rx_current_message) {
        const auto val =
            multiplayer_is_master() ? REG_SIOMULTI1 : REG_SIOMULTI0;
        if (mc.rx_current_all_zeroes and val) {
            mc.rx_current_all_zeroes = false;
        }
        mc.rx_current_message->data_[mc.rx_iter_state++] = val;

    } else {
        mc.rx_iter_state++;
    }
}


static bool multiplayer_busy()
{
    return REG_SIOCNT & SIO_START;
}


bool Platform::NetworkPeer::send_message(const Message& message)
{
    if (message.length_ > sizeof(TxInfo::data_)) {
        ::platform->fatal("invalid network packet size");
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

    auto& mc = multiplayer_comms;


    if (mc.tx_ring[mc.tx_ring_write_pos]) {
        // The writer does not seem to be keeping up! Guess we'll have to drop a
        // message :(
        mc.tx_loss += 1;

        auto lost_message = mc.tx_ring[mc.tx_ring_write_pos];
        mc.tx_ring[mc.tx_ring_write_pos] = nullptr;

        mc.tx_message_pool.post(lost_message);
    }

    auto msg = mc.tx_message_pool.get();
    if (not msg) {
        // error! Could not transmit messages fast enough, i.e. we've exhausted
        // the message pool! How to handle this condition!?
        mc.tx_loss += 1;
        return false;
    }

    __builtin_memcpy(msg->data_, message.data_, message.length_);

    mc.tx_ring[mc.tx_ring_write_pos] = msg;
    mc.tx_ring_write_pos += 1;
    mc.tx_ring_write_pos %= mc.tx_ring_size;

    return true;
}


static void multiplayer_tx_send()
{
    auto& mc = multiplayer_comms;

    if (mc.tx_iter_state == message_iters) {
        if (mc.tx_current_message) {
            mc.tx_message_pool.post(mc.tx_current_message);
            mc.tx_message_count += 1;
        }
        mc.tx_current_message = tx_ring_pop();
        mc.tx_iter_state = 0;
    }

    if (mc.tx_current_message) {
        REG_SIOMLT_SEND = mc.tx_current_message->data_[mc.tx_iter_state++];
    } else {
        mc.null_bytes_written += 2;
        mc.tx_iter_state++;
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
            ++multiplayer_comms.transmit_busy_count;
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

    multiplayer_comms.is_host = multiplayer_is_master();

    multiplayer_rx_receive();
    multiplayer_schedule_tx();
}


std::optional<Platform::NetworkPeer::Message>
Platform::NetworkPeer::poll_message()
{
    auto& mc = multiplayer_comms;

    if (mc.rx_iter_state == message_iters) {
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
        // P.S.: Tested on actual hardware, works fine.
        return {};
    }
    if (auto msg = rx_ring_pop()) {
        if (UNLIKELY(mc.poller_current_message not_eq nullptr)) {
            // failure to deallocate/consume message!
            mc.rx_message_pool.post(msg);
            disconnect();
            return {};
        }
        mc.poller_current_message = msg;
        return Platform::NetworkPeer::Message{
            reinterpret_cast<byte*>(msg->data_),
            static_cast<int>(sizeof(WireMessage::data_))};
    }
    return {};
}


void Platform::NetworkPeer::poll_consume(u32 size)
{
    auto& mc = multiplayer_comms;

    if (mc.poller_current_message) {
        mc.rx_message_pool.post(mc.poller_current_message);
    } else {
        ::platform->fatal("logic error in net poll");
    }
    mc.poller_current_message = nullptr;
}


static void __attribute__((noinline)) busy_wait(unsigned max)
{
    for (unsigned i = 0; i < max; i++) {
        __asm__ volatile("" : "+g"(i) : :);
    }
}


static void multiplayer_init()
{
    Microseconds delta = 0;

MASTER_RETRY:
    ::platform->network_peer().disconnect();

    ::platform->sleep(5);

    REG_RCNT = R_MULTI;
    REG_SIOCNT = SIO_MULTI;
    REG_SIOCNT |= SIO_IRQ | SIO_115200;

    irqEnable(IRQ_SERIAL);
    irqSet(IRQ_SERIAL, multiplayer_serial_isr);

    // Put this here for now, not sure whether it's really necessary...
    REG_SIOMLT_SEND = 0x5555;

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

    const char* handshake = "link__v00002";

    if (str_len(handshake) not_eq Platform::NetworkPeer::max_message_size) {
        ::platform->network_peer().disconnect();
        error(*::platform, "handshake string does not equal message size");
        return;
    }

    set_gflag(GlobalFlag::multiplayer_connected, true);

    ::platform->network_peer().send_message(
        {(byte*)handshake, sizeof handshake});

    multiplayer_schedule_tx();

    while (true) {
        ::platform->feed_watchdog();
        delta += ::platform->delta_clock().reset();
        if (delta > seconds(20)) {
            error(*::platform,
                  "no valid handshake received within a reasonable window");
            ::platform->network_peer().disconnect();
            return;
        } else if (auto msg = ::platform->network_peer().poll_message()) {
            for (u32 i = 0; i < sizeof handshake; ++i) {
                if (((u8*)msg->data_)[i] not_eq handshake[i]) {
                    if (multiplayer_is_master()) {
                        // For the master, if none of the other GBAs are in
                        // multi serial mode yet, the SIOCNT register will show
                        // that all gbas are in a ready state (all of one
                        // device). The master will, therefore, push out a
                        // message, and receive back garbage data. So we want to
                        // keep retrying, in order to account for the scenario
                        // where the other device is not yet plugged in, or the
                        // other player has not initiated their own connection.
                        info(*::platform, "master retrying...");

                        // busy-wait for a bit. This is sort of necessary;
                        // Platform::sleep() does not contribute to the
                        // delta clock offset (by design), so if we don't
                        // burn up some time here, we will take a _long_
                        // time to reach the timeout interval.
                        busy_wait(10000);
                        goto MASTER_RETRY; // lol yikes a goto
                    } else {
                        ::platform->network_peer().disconnect();
                        info(*::platform, "invalid handshake");
                        return;
                    }
                }
            }
            info(*::platform, "validated handshake");
            irqSet(IRQ_TIMER1, audio_update_fast_isr);
            ::platform->network_peer().poll_consume(sizeof handshake);
            return;
        }
    }
}


void Platform::NetworkPeer::connect(const char* peer)
{
    // If the gameboy player is active, any multiplayer initialization would
    // clobber the Normal_32 serial transfer between the gameboy player and the
    // gameboy advance.
    if (get_gflag(GlobalFlag::gbp_unlocked)) {
        return;
    }

    multiplayer_init();
}


void Platform::NetworkPeer::listen()
{
    if (get_gflag(GlobalFlag::gbp_unlocked)) {
        return;
    }

    multiplayer_init();
}


void Platform::NetworkPeer::update()
{
}


static int last_tx_count = 0;


Platform::NetworkPeer::Stats Platform::NetworkPeer::stats()
{
    auto& mc = multiplayer_comms;

    const int empty_transmits = mc.null_bytes_written / max_message_size;
    mc.null_bytes_written = 0;

    Float link_saturation = 0.f;

    if (empty_transmits) {
        auto tx_diff = mc.tx_message_count - last_tx_count;

        link_saturation = Float(tx_diff) / (empty_transmits + tx_diff);
    }

    last_tx_count = mc.tx_message_count;

    return {mc.tx_message_count,
            mc.rx_message_count,
            mc.tx_loss,
            mc.rx_loss,
            static_cast<int>(100 * link_saturation)};
}


bool Platform::NetworkPeer::supported_by_device()
{
    return true;
}


bool Platform::NetworkPeer::is_connected() const
{
    return get_gflag(GlobalFlag::multiplayer_connected);
}


bool Platform::NetworkPeer::is_host() const
{
    return multiplayer_comms.is_host;
}


void Platform::NetworkPeer::disconnect()
{
    // Be very careful editing this function. We need to get ourselves back to a
    // completely clean slate, otherwise, we won't be able to reconnect (e.g. if
    // you leave a message sitting in the transmit ring, it may be erroneously
    // sent out when you try to reconnect, instead of the handshake message);
    if (is_connected()) {

        irqSet(IRQ_TIMER1, audio_update_fast_isr);

        info(*::platform, "disconnected!");
        set_gflag(GlobalFlag::multiplayer_connected, false);
        irqDisable(IRQ_SERIAL);
        if (multiplayer_is_master()) {
            enable_watchdog();
        }
        REG_SIOCNT = 0;

        auto& mc = multiplayer_comms;

        if (mc.poller_current_message) {
            // Not sure whether this is the correct thing to do here...
            mc.rx_message_pool.post(mc.poller_current_message);
            mc.poller_current_message = nullptr;
        }

        mc.rx_iter_state = 0;
        if (mc.rx_current_message) {
            mc.rx_message_pool.post(mc.rx_current_message);
            mc.rx_current_message = nullptr;
        }
        mc.rx_current_all_zeroes = true;
        for (auto& msg : mc.rx_ring) {
            if (msg) {
                mc.rx_message_pool.post(msg);
                msg = nullptr;
            }
        }
        mc.rx_ring_write_pos = 0;
        mc.rx_ring_read_pos = 0;

        mc.tx_iter_state = 0;
        if (mc.tx_current_message) {
            mc.tx_message_pool.post(mc.tx_current_message);
            mc.tx_current_message = nullptr;
        }
        for (auto& msg : mc.tx_ring) {
            if (msg) {
                mc.tx_message_pool.post(msg);
                msg = nullptr;
            }
        }
        mc.tx_ring_write_pos = 0;
        mc.tx_ring_read_pos = 0;
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
        GPIO_PORT_DATA = (temp << 1) | 4;
        GPIO_PORT_DATA = (temp << 1) | 4;
        GPIO_PORT_DATA = (temp << 1) | 4;
        GPIO_PORT_DATA = (temp << 1) | 5;
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
        GPIO_PORT_DATA = (temp << 1) | 4;
        GPIO_PORT_DATA = (temp << 1) | 4;
        GPIO_PORT_DATA = (temp << 1) | 4;
        GPIO_PORT_DATA = (temp << 1) | 5;
    }
}


static u8 rtc_gpio_read_value()
{
    u8 temp;
    u8 value = 0;

    for (u8 i = 0; i < 8; i++) {
        GPIO_PORT_DATA = 4;
        GPIO_PORT_DATA = 4;
        GPIO_PORT_DATA = 4;
        GPIO_PORT_DATA = 4;
        GPIO_PORT_DATA = 4;
        GPIO_PORT_DATA = 5;

        temp = ((GPIO_PORT_DATA & 2) >> 1);
        value = (value >> 1) | (temp << 7);
    }

    return value;
}


static u8 rtc_get_status()
{
    GPIO_PORT_DATA = 1;
    GPIO_PORT_DATA = 5;
    GPIO_PORT_DIRECTION = 7;

    rtc_gpio_write_command(S3511A_CMD_STATUS | S3511A_RD);

    GPIO_PORT_DIRECTION = 5;

    const auto status = rtc_gpio_read_value();

    GPIO_PORT_DATA = 1;
    GPIO_PORT_DATA = 1;

    return status;
}


static auto rtc_get_datetime()
{
    std::array<u8, 7> result;

    GPIO_PORT_DATA = 1;
    GPIO_PORT_DATA = 5;
    GPIO_PORT_DIRECTION = 7;

    rtc_gpio_write_command(S3511A_CMD_DATETIME | S3511A_RD);

    GPIO_PORT_DIRECTION = 5;

    for (auto& val : result) {
        val = rtc_gpio_read_value();
    }

    result[4] &= 0x7F;

    GPIO_PORT_DATA = 1;
    GPIO_PORT_DATA = 1;

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
    if (get_gflag(GlobalFlag::rtc_faulty)) {
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
    GPIO_PORT_READ_ENABLE = 1;

    auto status = rtc_get_status();
    if (status & S3511A_STATUS_POWER) {
        warning(pfrm, "RTC chip power failure");
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// RemoteConsole
//
// Thanks to Ties Stuij, who created a great tutorial about how to correctly
// wire the UART to the GBA. Note that we are not using flow control, so RTS and
// CTS do not need to be connected.
//
// GBA                rs232 usb cable
// ---                ---------------
// 2 SO  red --------> 5 RxD yel
// 3 SI  org <-------- 4 TxD org
// 4 SD  bwn --------> 2 CTS bwn
// 5 SC  grn <-------- 6 RTS grn
// 6 GND blu <-------> 1 GND blk
//
////////////////////////////////////////////////////////////////////////////////


using ConsoleLine = Platform::RemoteConsole::Line;


struct RemoteConsoleState {
    // NOTE: Some of these vars should probably be volatile, but, as we're
    // dealing with keystrokes, which happen on a human timescale, the
    // probability of anything getting messed up is pretty small.

    std::optional<DynamicMemory<ConsoleLine>> rx_in_progress_;
    Buffer<DynamicMemory<ConsoleLine>, 4> rx_full_lines_;

    std::optional<DynamicMemory<ConsoleLine>> tx_msg_;
};


static std::optional<DynamicMemory<RemoteConsoleState>> remote_console_state;


static void uart_serial_isr()
{
    auto& state = **::remote_console_state;

    const char data = REG_SIODATA8;

    const bool is_uart_error = REG_SIOCNT & (1 << 6);
    if (is_uart_error) {
        // just for debugging purposes
        ++multiplayer_comms.rx_loss;
        return;
    }

    const bool send_ready = !(REG_SIOCNT & 0x0010);
    if (send_ready and state.tx_msg_ and not(*state.tx_msg_)->empty()) {

        // just for debugging purposes
        ++multiplayer_comms.tx_message_count;

        REG_SIODATA8 = *((*state.tx_msg_)->end() - 1);
        (*state.tx_msg_)->pop_back();
        return;
    } else if (state.tx_msg_ and (*state.tx_msg_)->empty()) {
        state.tx_msg_.reset();
    }

    // FIXME: I'm not seeing the receive_ready flag for some reason, but this
    // code really _should_ be checking the state of the receive flag in the
    // serial control register before reading a byte. What I'm doing here is
    // possibly undefined behavior that happens to work, because my code is
    // ignoring the null bytes.
    //
    // const bool is_rcv = !(REG_SIOCNT & 0x0020);
    // if (is_rcv) {

    // just for debugging purposes
    ++multiplayer_comms.rx_message_count;

    if (data == '\r') {
        if (state.rx_in_progress_) {
            state.rx_full_lines_.push_back(std::move(*state.rx_in_progress_));
            state.rx_in_progress_.reset();
        }
    } else if (data == 8 /* ASCII backspace */ or
               data == 0x7f /* Strange char used by picocom as a backspace */) {
        if (state.rx_in_progress_) {
            (*state.rx_in_progress_)->pop_back();
        }
    } else {
        if (not state.rx_in_progress_) {
            state.rx_in_progress_ = allocate_dynamic<ConsoleLine>(*::platform);
        }

        if (state.rx_in_progress_) {
            (*state.rx_in_progress_)->push_back(data);
        }
    }

    // }
}


static void start_remote_console(Platform& pfrm)
{
    ::remote_console_state = allocate_dynamic<RemoteConsoleState>(pfrm);

    irqEnable(IRQ_SERIAL);

    irqSet(IRQ_SERIAL, uart_serial_isr);

    // Stick a character in the data register. This stops the GBA transmitting a
    // Character as soon as it goes into UART mode (?!?)
    REG_SIODATA8 = 'A';

    // Now to go into UART mode
    REG_RCNT = 0;
    REG_SIOCNT = 0;

    // NOTE: see gba.h for constants
    REG_SIOCNT = SIO_9600 | SIO_UART_LENGTH_8 | SIO_UART_SEND_ENABLE |
                 SIO_UART_RECV_ENABLE | SIO_UART | SIO_IRQ;
}


auto Platform::RemoteConsole::readline() -> std::optional<Line>
{
    auto& state = **::remote_console_state;

    if (not state.rx_full_lines_.empty()) {
        auto ret = std::move(*state.rx_full_lines_.begin());

        state.rx_full_lines_.erase(state.rx_full_lines_.begin());

        return *ret;
    }
    return {};
}


bool Platform::RemoteConsole::printline(const char* text, bool show_prompt)
{
    auto& state = **::remote_console_state;

    if (state.tx_msg_ and not(*state.tx_msg_)->empty()) {
        // TODO: add a queue for output messages! At the moment, there's already
        // a message being written, so we'll need to ignore the input.
        return false;
    }

    state.tx_msg_ = allocate_dynamic<ConsoleLine>(*::platform);

    std::optional<char> first_char;

    if (*text not_eq '\0') {
        first_char = *text;
        ++text;
    }

    while (*text not_eq '\0') {
        // yeah, this isn't great. We're inserting chars into our buffer in
        // reverse order in the printline call, so that we can simply call
        // pop_back(), which is a constant-time operation, in the more
        // time-critical interrupt handler. Every insert() will essentially
        // memmove what's in the buffer by one byte to the right.
        (*state.tx_msg_)->insert((*state.tx_msg_)->begin(), *text);
        ++text;
    }

    (*state.tx_msg_)->insert((*state.tx_msg_)->begin(), '\r');
    (*state.tx_msg_)->insert((*state.tx_msg_)->begin(), '\n');

    if (show_prompt) {
        // Show the prompt
        (*state.tx_msg_)->insert((*state.tx_msg_)->begin(), '>');
        (*state.tx_msg_)->insert((*state.tx_msg_)->begin(), ' ');
    }

    if (first_char) {
        // Now that we're done copying the output message, place the first
        // character in the uart shift register, to kick off the send.
        REG_SIODATA8 = *first_char;
    }

    return true;
}


void Platform::enable_feature(const char* feature_name, int value)
{
    if (str_cmp(feature_name, "gswap") == 0) {
        *((u16*)0x4000002) = 0x0000 | (bool)value;
    } else if (str_cmp(feature_name, "spatialized-audio") == 0) {
        switch (value) {
        case 0:
            irqSet(IRQ_TIMER1, audio_update_fast_isr);
            break;

        case 1:
            irqSet(IRQ_TIMER1, audio_update_spatialized_isr);
            break;

        case 2:
            irqSet(IRQ_TIMER1, audio_update_spatialized_stereo_isr);
            break;
        }
    }
}


#endif // __GBA__
