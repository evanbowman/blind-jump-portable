
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
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>


PSP_MODULE_INFO("Blind Jump", 0, 1, 0);


void start(Platform& pf);
// {
//     while (pf.is_running()) {
//         pf.keyboard().poll();

//         const auto delta = pf.delta_clock().reset();
//     }
// }


int main(int argc, char** argv)
{
    Platform pf;

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


static SDL_Window* window;
static SDL_Renderer* renderer;


Platform::Platform()
{
    setup_callbacks();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);


    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    window = SDL_CreateWindow("Active Window",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 240, 160, 0);

    if (window == nullptr) {
        fatal("failed to create window");
    }

    renderer = SDL_CreateRenderer(window, -1, 0);

    if (renderer == nullptr) {
        fatal("failed to create renderer");
    }
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
    // ...
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
    // TODO
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
    return seconds(Float(1) / 60);
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
    // TODO
    static const Vec2<u32> gba_widescreen{240, 160};
    return gba_widescreen;
}


void Platform::Screen::clear()
{
    for (auto it = task_queue_.begin(); it not_eq task_queue_.end();) {
        (*it)->run();
        if ((*it)->complete()) {
            (*it)->running_ = false;
            task_queue_.erase(it);
        } else {
            ++it;
        }
    }

    // TODO: VSYNC here?
}


void Platform::Screen::draw(const Sprite& spr)
{
    // TODO...
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


void Platform::Screen::display()
{
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawPoint(renderer, 20, 20);
    SDL_RenderPresent(renderer);
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
    // TODO
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

    if (buttonInput.Buttons) {
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
    }

    if (UNLIKELY(static_cast<bool>(::missed_keys))) {
        for (int i = 0; i < (int)Key::count; ++i) {
            if ((*::missed_keys)[i]) {
                states_[i] = true;
            }
        }
        ::missed_keys.reset();
    }
}
