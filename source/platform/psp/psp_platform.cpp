
////////////////////////////////////////////////////////////////////////////////
//
//
// PSP Platform
//
//
////////////////////////////////////////////////////////////////////////////////

#include "platform/platform.hpp"
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspkernel.h>
#include <pspsystimer.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <png.h>
#include <stdio.h>
#include <memory>
#include "localization.hpp"


extern "C" {
#include "glib2d.h"
}



PSP_MODULE_INFO("Blind Jump", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);


void start(Platform& pf);
// {
//     while (pf.is_running()) {
//         pf.keyboard().poll();

//         const auto delta = pf.delta_clock().reset();
//     }
// }


Platform* platform;


int main(int argc, char** argv)
{
    // pspDebugScreenInit();

    Platform pf;
    ::platform = &pf;

    start(pf);

    return 0;
}


static bool is_running = true;


int exit_callback(int arg1, int arg2, void* common)
{
    sceKernelExitGame();
    is_running = false;
    return 0;
}


int callback_thread(SceSize args, void* argp)
{
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();

    return 0;
}


int setup_callbacks(void)
{
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
    return thid;
}


bool Platform::is_running() const
{
    return ::is_running;
}


// FIXME!
#include "../../../external/pngle/pngle.h"


extern "C" {
// FIXME!
#include "../../../build/psp/spritesheet.c"

}


struct SpriteMemory {
    alignas(16) g2dColor pixels_[64 * 64];
};


SpriteMemory sprite_image_ram[200];


Platform::Platform()
{
    setup_callbacks();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}


static ObjectPool<RcBase<ScratchBuffer,
                         scratch_buffer_count>::ControlBlock,
                  scratch_buffer_count> scratch_buffer_pool;


static int scratch_buffers_in_use = 0;
static int scratch_buffer_highwater = 0;


ScratchBufferPtr Platform::make_scratch_buffer()
{
    auto finalizer = [](RcBase<ScratchBuffer,
                               scratch_buffer_count>::ControlBlock* ctrl) {
        --scratch_buffers_in_use;
        ctrl->pool_->post(ctrl);
    };

    auto maybe_buffer = Rc<ScratchBuffer, scratch_buffer_count>::create(
        &scratch_buffer_pool, finalizer);
    if (maybe_buffer) {
        ++scratch_buffers_in_use;
        if (scratch_buffers_in_use > scratch_buffer_highwater) {
            scratch_buffer_highwater = scratch_buffers_in_use;

            // StringBuffer<60> str = "sbr highwater: ";

            // str += to_string<10>(scratch_buffer_highwater).c_str();

            // info(*::platform, str.c_str());
        }
        return *maybe_buffer;
    } else {
        // screen().fade(1.f, ColorConstant::electric_blue);
        fatal("scratch buffer pool exhausted");
        while (true) ;
    }
}


int Platform::scratch_buffers_remaining()
{
    return scratch_buffer_count - scratch_buffers_in_use;
}


static Buffer<Platform::Task*, 7> task_queue_;


void Platform::push_task(Task* task)
{
    task->complete_ = false;
    task->running_ = true;

    if (not task_queue_.push_back(task)) {
        // error(*this, "failed to enqueue task");
        while (true)
            ;
    }
}


void Platform::feed_watchdog()
{
    // TODO
}


Platform::~Platform()
{
}


void Platform::sleep(Frame frames)
{
    while (frames) {
        --frames;
        sceDisplayWaitVblankStart();
    }
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


static ObjectPool<RcBase<Platform::DynamicTexture,
                         Platform::dynamic_texture_count>::ControlBlock,
                  Platform::dynamic_texture_count>
    dynamic_texture_pool;


void Platform::DynamicTexture::remap(u16 spritesheet_offset)
{
}


std::optional<DateTime> Platform::startup_time() const
{
    return {};
}


bool Platform::NetworkPeer::supported_by_device()
{
    return false;
}


Platform::NetworkPeer::Stats Platform::NetworkPeer::stats()
{
    return {0, 0, 0, 0, 0};
}


std::optional<Platform::DynamicTexturePtr> Platform::make_dynamic_texture()
{
    auto finalizer =
        [](RcBase<Platform::DynamicTexture,
                  Platform::dynamic_texture_count>::ControlBlock* ctrl) {
            // Nothing managed, so nothing to do.
            dynamic_texture_pool.post(ctrl);
        };

    auto dt = DynamicTexturePtr::create(&dynamic_texture_pool, finalizer, 0);
    if (dt) {
        return *dt;
    }

    warning(*this, "Failed to allocate DynamicTexture.");
    return {};
}


void Platform::load_sprite_texture(const char* name)
{
    StringBuffer<64> str_name = name;

    auto pngle = pngle_new();
    if (pngle == nullptr) {
        fatal("failed to create png loader context");
    }

    pngle_set_draw_callback(pngle, [](pngle_t *pngle,
                                      u32 x,
                                      u32 y,
                                      u32 w,
                                      u32 h,
                                      u8 rgba[4]) {
        auto target_sprite = x / 32;
        auto color = G2D_RGBA(rgba[0], rgba[1], rgba[2], rgba[3]);

        if (rgba[0] == 0xFF and rgba[2] == 0xFF) {
            color = G2D_RGBA(255, 0, 255, 0);
        }

        auto& spr = sprite_image_ram[target_sprite];

        auto set_pixel = [&](int x, int y) {
            spr.pixels_[x % 64 + y * 64] = color;
        };

        set_pixel((x * 2), (y * 2));
        set_pixel((x * 2) + 1, (y * 2));
        set_pixel((x * 2), (y * 2) + 1);
        set_pixel((x * 2) + 1, (y * 2) + 1);
    });

    int remaining = size_spritesheet;
    auto buf = spritesheet;
    while (remaining) {
        auto status = pngle_feed(pngle, buf, remaining);
        if (status == -1) {
            StringBuffer<64> err("pngle error: ");
            err += pngle_error(pngle);
            fatal(err.c_str());
        }
        remaining -= status;
        buf += status;
    }

    pngle_destroy(pngle);
}


void Platform::load_tile0_texture(const char* name)
{
    // TODO
}


void Platform::load_tile1_texture(const char* name)
{
    // TODO
}


void Platform::load_overlay_texture(const char* name)
{
    // TODO
}


void Platform::fill_overlay(u16 TileDesc)
{
    // TODO
}


void Platform::set_overlay_origin(Float x, Float y)
{
    // TODO
}


void Platform::set_tile(Layer layer, u16 x, u16 y, u16 val)
{
    // TODO
}


void Platform::set_tile(u16 x, u16 y, TileDesc glyph, const FontColors& colors)
{
    // TODO
}


u16 Platform::get_tile(Layer layer, u16 x, u16 y)
{
    // TODO
    return 0;
}


Platform::DeviceName Platform::device_name() const
{
    return "SonyPSP";
}


void Platform::fatal(const char* msg)
{
    pspDebugScreenInit();
    pspDebugScreenPrintf(msg);
    while (true) ;
}


const char* Platform::get_opt(char opt)
{
    return nullptr;
}


// FIXME...
#include "platform/gba/files.cpp"


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


void Platform::enable_glyph_mode(bool enabled)
{
    // ...
}


TileDesc Platform::map_glyph(const utf8::Codepoint& glyph,
                             TextureCpMapper mapper)
{
    // TODO
    return 111;
}


bool Platform::read_save_data(void* buffer, u32 data_length, u32 offset)
{
    // TODO
    return false;
}


bool Platform::write_save_data(const void* data, u32 length, u32 offset)
{
    // TODO
    return false;
}


////////////////////////////////////////////////////////////////////////////////
// SystemClock
////////////////////////////////////////////////////////////////////////////////


Platform::SystemClock::SystemClock()
{
}


std::optional<DateTime> Platform::SystemClock::now()
{
    return {};
}


////////////////////////////////////////////////////////////////////////////////
// NetworkPeer
////////////////////////////////////////////////////////////////////////////////


Platform::NetworkPeer::NetworkPeer()
{
}


bool Platform::NetworkPeer::is_host() const
{
    // TODO
    return false;
}


void Platform::NetworkPeer::disconnect()
{
    // TODO
}


void Platform::NetworkPeer::connect(const char* peer_address)
{
    // TODO
}


void Platform::NetworkPeer::listen()
{
    // TODO
}


Platform::NetworkPeer::Interface Platform::NetworkPeer::interface() const
{
    return Interface::serial_cable;
}


bool Platform::NetworkPeer::is_connected() const
{
    // TODO
    return false;
}


bool Platform::NetworkPeer::send_message(const Message&)
{
    // TODO
    return true;
}


void Platform::NetworkPeer::update()
{
    // TODO
}


auto Platform::NetworkPeer::poll_message() -> std::optional<Message>
{
    // TODO
    return {};
}


void Platform::NetworkPeer::poll_consume(u32 length)
{
    // TODO
}


Platform::NetworkPeer::~NetworkPeer()
{
}


////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


Platform::DeltaClock::DeltaClock()
{
}


Microseconds Platform::DeltaClock::reset()
{
    // FIXME
    return (Float(1) / 60) * 1000000;
}


Platform::DeltaClock::~DeltaClock()
{
}


////////////////////////////////////////////////////////////////////////////////
// Screen
////////////////////////////////////////////////////////////////////////////////


Platform::Screen::Screen()
{
}


Vec2<u32> Platform::Screen::size() const
{
    // We upsample the textures by 2x on the psp, because the pixels are so
    // small.
    static const Vec2<u32> psp_half_widescreen{240, 136};
    return psp_half_widescreen;
}


void Platform::Screen::clear()
{
    g2dClear(G2D_RGBA(128, 128, 128, 255));

    for (auto it = task_queue_.begin(); it not_eq task_queue_.end();) {
        (*it)->run();
        if ((*it)->complete()) {
            (*it)->running_ = false;
            task_queue_.erase(it);
        } else {
            ++it;
        }
    }
}


static Buffer<Sprite, 128> sprite_queue;


void Platform::Screen::draw(const Sprite& spr)
{
    sprite_queue.push_back(spr);
}


void Platform::Screen::fade(Float amount,
                            ColorConstant,
                            std::optional<ColorConstant>,
                            bool,
                            bool)
{
    // TODO
}


void Platform::Screen::pixelate(u8 amount,
                                bool include_overlay,
                                bool include_background,
                                bool include_sprites)
{
    // TODO
}


static g2dColor make_color(int color_hex, u8 alpha)
{
    const auto r = (color_hex & 0xFF0000) >> 16;
    const auto g = (color_hex & 0x00FF00) >> 8;
    const auto b = (color_hex & 0x0000FF);

    return G2D_RGBA(r, g, b, alpha);
}


static void display_sprite(const Platform::Screen& screen, const Sprite& spr)
{
    // FIXME: support translucent sprites
    if (spr.get_alpha() == Sprite::Alpha::transparent) {
        return;
    }

    g2dTexture temp;
    temp.tw = 64;
    temp.th = 64;
    temp.h = 64;
    temp.swizzled = false;

    int offset = 0;
    int width = 64;

    // FIXME: Flipping flags! What to do about these? Maybe there's some way to
    // easily do this by messing around with the vertices.

    if (spr.get_size() == Sprite::Size::w32_h32) {
        temp.data = sprite_image_ram[spr.get_texture_index()].pixels_;
    } else {
        // if (spr.get_texture_index() % 2) {

        // }
        width = 32;
        temp.data = sprite_image_ram[spr.get_texture_index() / 2].pixels_;
    }

    temp.w = width;

    const auto view_center = screen.get_view().get_center();

    const auto pos = spr.get_position() - spr.get_origin().cast<Float>();

    auto abs_position = pos - view_center;

    if (spr.get_flip().x) {
        abs_position.x += width / 2;
    }

    if (spr.get_flip().y) {
        abs_position.y += 32;
    }

    abs_position.x *= 2.f;
    abs_position.y *= 2.f;

    g2dBeginRects(&temp);
    g2dSetCoordMode(G2D_UP_LEFT);
    g2dSetCoordXY(abs_position.x, abs_position.y);
    if (spr.get_size() not_eq Sprite::Size::w32_h32) {
        if (spr.get_texture_index() % 2) {
            g2dSetCropXY(32, 0);
            g2dSetCropWH(32, 64);
        } else {
            g2dSetCropXY(0, 0);
            g2dSetCropWH(32, 64);
        }
    }
    if (spr.get_alpha() == Sprite::Alpha::translucent) {
        g2dSetAlpha(128);
    } else {
        g2dSetAlpha(255);
    }
    g2dSetRotation(0);
    if (spr.get_mix().color_ not_eq ColorConstant::null and
        spr.get_mix().amount_ == 255) {
        g2dSetColor(make_color((int)spr.get_mix().color_,
                               spr.get_mix().amount_));
    } else {
        g2dResetColor();
    }
    if (spr.get_flip().x or spr.get_flip().y) {
        Float x_scale = 1.f;
        Float y_scale = 1.f;

        if (spr.get_flip().x) {
            x_scale = -1;
        }

        if (spr.get_flip().y) {
            y_scale = -1;
        }

        g2dSetScale(x_scale, y_scale);

    } else {
        g2dResetScale();
    }
    g2dAdd();
    g2dEnd();
    g2dResetCrop();
}


void Platform::Screen::display()
{
    for (auto& spr : reversed(::sprite_queue)) {
        display_sprite(*this, spr);
    }

    sprite_queue.clear();

    g2dFlip(G2D_VSYNC);
}


void Platform::Screen::set_contrast(Contrast c)
{
    // TODO...
}


Contrast Platform::Screen::get_contrast() const
{
    return 0; // TODO...
}


void Platform::Screen::enable_night_mode(bool)
{
    // TODO...
}


////////////////////////////////////////////////////////////////////////////////
// Speaker
////////////////////////////////////////////////////////////////////////////////


Platform::Speaker::Speaker()
{
}


void Platform::Speaker::play_sound(const char* name,
                                   int priority,
                                   std::optional<Vec2<Float>> position)
{
    // TODO
}


bool Platform::Speaker::is_sound_playing(const char* name)
{
    // TODO
    return false;
}


void Platform::Speaker::set_position(const Vec2<Float>& position)
{
    // TODO
}


Microseconds Platform::Speaker::track_length(const char* sound_or_music_name)
{
    // TODO
    return 0;
}


void Platform::Speaker::play_music(const char* name, Microseconds offset)
{
    // TODO
}


void Platform::Speaker::stop_music()
{
    // TODO
}


bool Platform::Speaker::is_music_playing(const char* name)
{
    // TODO
    return false;
}


////////////////////////////////////////////////////////////////////////////////
// Logger
////////////////////////////////////////////////////////////////////////////////


static Severity log_threshold;


void Platform::Logger::set_threshold(Severity severity)
{
    log_threshold = severity;
}


void Platform::Logger::read(void* buffer, u32 start_offset, u32 num_bytes)
{
}


void Platform::on_watchdog_timeout(WatchdogCallback callback)
{
    // TODO
}


Platform::Logger::Logger()
{
}


////////////////////////////////////////////////////////////////////////////////
// RemoteConsole
////////////////////////////////////////////////////////////////////////////////


bool Platform::RemoteConsole::printline(const char* line)
{
    //    pspDebugScreenPrintf(line);
    return false;
}


auto Platform::RemoteConsole::readline() -> std::optional<Line>
{
    // TODO
    return {};
}


////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////


std::optional<Bitvector<int(Key::count)>> missed_keys;


void Platform::Keyboard::rumble(bool)
{
    // PSP hardware does not support rumble/vibrate.
}


void Platform::Keyboard::register_controller(const ControllerInfo& info)
{
    // ...
}


void Platform::Keyboard::poll()
{
    std::copy(std::begin(states_), std::end(states_), std::begin(prev_));

    SceCtrlData buttonInput;

    sceCtrlPeekBufferPositive(&buttonInput, 1);

    states_[int(Key::start)] = buttonInput.Buttons & PSP_CTRL_START;
    states_[int(Key::select)] = buttonInput.Buttons & PSP_CTRL_SELECT;
    states_[int(Key::right)] = buttonInput.Buttons & PSP_CTRL_RIGHT;
    states_[int(Key::left)] = buttonInput.Buttons & PSP_CTRL_LEFT;
    states_[int(Key::down)] =  buttonInput.Buttons & PSP_CTRL_DOWN;
    states_[int(Key::up)] = buttonInput.Buttons & PSP_CTRL_UP;
    states_[int(Key::alt_1)] = buttonInput.Buttons & PSP_CTRL_RTRIGGER;
    states_[int(Key::alt_2)] = buttonInput.Buttons & PSP_CTRL_LTRIGGER;
    states_[int(Key::action_1)] = buttonInput.Buttons & PSP_CTRL_CROSS;
    states_[int(Key::action_2)] = buttonInput.Buttons & PSP_CTRL_CIRCLE;


    if (UNLIKELY(static_cast<bool>(::missed_keys))) {
        for (int i = 0; i < (int)Key::count; ++i) {
            if ((*::missed_keys)[i]) {
                states_[i] = true;
            }
        }
        ::missed_keys.reset();
    }
}
