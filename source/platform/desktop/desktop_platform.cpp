#include "number/random.hpp"
#include "platform/platform.hpp"
#include "script/lisp.hpp"


////////////////////////////////////////////////////////////////////////////////
//
//
// Desktop Platform
//
//
////////////////////////////////////////////////////////////////////////////////


#ifdef _WIN32
#define PATH_DELIMITER "\\"
#else
#define PATH_DELIMITER "/"
#endif


#include "SFML/Audio.hpp"
#include "SFML/Graphics.hpp"
#include "SFML/Network.hpp"
#include "SFML/System.hpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
// The game logic and graphics used to run on different threads. But the game is
// efficient enough to run on a gameboy, so there isn't really any need for
// threading.
// #include <mutex>
#include <popl/popl.hpp>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_map>


Platform::DeviceName Platform::device_name() const
{
    return "Desktop";
}


std::string resource_path();


////////////////////////////////////////////////////////////////////////////////
// TileMap
////////////////////////////////////////////////////////////////////////////////


class TileMap : public sf::Drawable, public sf::Transformable {
public:
    TileMap(sf::Texture* texture, sf::Vector2u tile_size, int width, int height)
        : texture_(texture), tile_size_(tile_size), width_(width),
          height_(height)
    {
        // resize the vertex array to fit the level size
        vertices_.setPrimitiveType(sf::Quads);
        vertices_.resize(width * height * 4);
    }

    void set_tile(int x, int y, int index)
    {
        if (x >= width_ or y >= height_) {
            return;
        }

        if (texture_->getSize().x == 0 or texture_->getSize().y == 0 or
            tile_size_.x == 0 or tile_size_.y == 0) {
            return;
        }

        // find tile position in the tileset texture
        int tu = index % (texture_->getSize().x / tile_size_.x);
        int tv = index / (texture_->getSize().x / tile_size_.x);

        // get a pointer to the current tile's quad
        sf::Vertex* quad = &vertices_[(x + y * width_) * 4];

        // define its 4 corners
        quad[0].position = sf::Vector2f(x * tile_size_.x, y * tile_size_.y);
        quad[1].position =
            sf::Vector2f((x + 1) * tile_size_.x, y * tile_size_.y);
        quad[2].position =
            sf::Vector2f((x + 1) * tile_size_.x, (y + 1) * tile_size_.y);
        quad[3].position =
            sf::Vector2f(x * tile_size_.x, (y + 1) * tile_size_.y);

        // define its 4 texture coordinates
        quad[0].texCoords = sf::Vector2f(tu * tile_size_.x, tv * tile_size_.y);
        quad[1].texCoords =
            sf::Vector2f((tu + 1) * tile_size_.x, tv * tile_size_.y);
        quad[2].texCoords =
            sf::Vector2f((tu + 1) * tile_size_.x, (tv + 1) * tile_size_.y);
        quad[3].texCoords =
            sf::Vector2f(tu * tile_size_.x, (tv + 1) * tile_size_.y);
    }

    Vec2<int> size() const
    {
        return {width_, height_};
    }

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        states.transform *= getTransform();

        states.texture = texture_;

        target.draw(vertices_, states);
    }

    sf::VertexArray vertices_;
    sf::Texture* texture_;
    sf::Vector2u tile_size_;
    const int width_;
    const int height_;
};


////////////////////////////////////////////////////////////////////////////////
// Global State Data
////////////////////////////////////////////////////////////////////////////////


static constexpr Vec2<u32> resolution{240, 160};


static const TileDesc glyph_region_start = 504;


class Platform::Data {
public:
    sf::Texture spritesheet_texture_;
    sf::Texture tile0_texture_;
    sf::Texture tile1_texture_;
    sf::Texture overlay_texture_;
    sf::Texture background_texture_;
    sf::Shader color_shader_;

    std::map<utf8::Codepoint, TileDesc> glyph_table_;
    TileDesc next_glyph_ = glyph_region_start;

    Vec2<Float> overlay_origin_;

    sf::RectangleShape fade_overlay_;
    bool fade_include_sprites_ = true;
    bool fade_include_overlay_ = false;

    sf::Color fade_color_ = sf::Color::Transparent;

    TileMap overlay_;
    TileMap map_0_;
    TileMap map_1_;
    TileMap background_;

    bool map_0_changed_ = false;
    bool map_1_changed_ = false;
    bool background_changed_ = false;

    sf::RenderTexture map_0_rt_;
    sf::RenderTexture map_1_rt_;
    sf::RenderTexture background_rt_;

    const bool fullscreen_;
    int window_scale_ = 2;
    sf::RenderWindow window_;
    sf::RenderTexture rt_;

    sf::Texture vignette_texture_;
    bool show_vignette_ = false;

    Vec2<u32> window_size_;

    // std::mutex audio_lock_;
    sf::Music music_;
    std::list<sf::Sound> sounds_;
    std::map<std::string, sf::SoundBuffer> sound_data_;


    Data(Platform& pfrm)
        : overlay_(&overlay_texture_, {8, 8}, 32, 32),
          map_0_(&tile0_texture_, {32, 24}, 16, 20),
          map_1_(&tile1_texture_, {32, 24}, 16, 20),
          background_(&background_texture_, {8, 8}, 32, 32),
          fullscreen_( // lisp::loadv<lisp::Integer>("fullscreen").value_
              false),
          window_scale_([&] {
              if (fullscreen_) {
                  auto ssize = sf::VideoMode::getDesktopMode();
                  auto x_scale = ssize.width / resolution.x;
                  auto y_scale = ssize.height / resolution.y;
                  return static_cast<int>(std::max(x_scale, y_scale));
              } else {
                  return
                      // lisp::loadv<lisp::Integer>("window-scale").value_
                      3;
              }
          }()),
          window_(sf::VideoMode(resolution.x * window_scale_,
                                resolution.y * window_scale_),
                  "Blind Jump",
                  [&] {
                      if (fullscreen_) {
                          return static_cast<int>(sf::Style::Fullscreen);
                      } else {
                          return sf::Style::Titlebar | sf::Style::Close |
                                 sf::Style::Resize;
                      }
                  }())
    {
        window_.setVerticalSyncEnabled(true);
        // window_.setMouseCursorVisible(false);

        window_size_ = {(u32)resolution.x * window_scale_,
                        (u32)resolution.y * window_scale_};

        rt_.create(240, 160);

        const auto vignette_path =
            resource_path() + ("images" PATH_DELIMITER) + "vignette.png";
        if (not vignette_texture_.loadFromFile(vignette_path)) {
            error(pfrm, "failed to load vignette texture");
        }

        auto sound_folder = resource_path() + ("sounds" PATH_DELIMITER);
        // lisp::loadv<lisp::Symbol>("sound-dir").name_;

        for (auto& dirent : std::filesystem::directory_iterator(sound_folder)) {
            const auto filename = dirent.path().stem().string();
            static const std::string prefix("sound_");
            const auto prefix_loc = filename.find(prefix);

            if (prefix_loc not_eq std::string::npos) {
                std::ifstream file_data(dirent.path(), std::ios::binary);
                std::vector<s8> buffer(
                    std::istreambuf_iterator<char>(file_data), {});

                // The gameboy advance sound data was 8 bit signed mono at
                // 16kHz. Here, we're upsampling to 16bit signed.
                std::vector<s16> upsampled;

                for (s8 sample : buffer) {
                    upsampled.push_back(sample << 8);
                }

                sf::SoundBuffer sound_buffer;
                sound_buffer.loadFromSamples(
                    upsampled.data(), upsampled.size(), 1, 16000);

                sound_data_[filename.substr(prefix.size())] = sound_buffer;
            }
        }
    }
};


static Platform* platform = nullptr;


class WatchdogTask : public Platform::Task {
public:
    WatchdogTask()
    {
        total_ = 0;
        last_ = std::chrono::high_resolution_clock::now();
    }

    void run() override
    {
        if (::platform->data() and not ::platform->is_running()) {
            Task::completed();
            return;
        }

        const auto now = std::chrono::high_resolution_clock::now();
        total_ +=
            std::chrono::duration_cast<std::chrono::microseconds>(now - last_)
                .count();

        last_ = now;

        if (total_ > seconds(10)) {
            if (::platform->data()) {
                if (::platform->data()->window_.isOpen()) {
                    ::platform->data()->window_.close();
                }
            }

            error(*::platform, "unresponsive process killed by watchdog");

            // Give all of the threads the opportunity to receive the window
            // shutdown sequence and exit responsibly.
            std::this_thread::sleep_for(std::chrono::seconds(1));

            exit(EXIT_FAILURE);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    void feed()
    {
        total_ = 0;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> last_;
    std::atomic<Microseconds> total_;

} watchdog_task;


void Platform::enable_feature(const char* feature_name, bool enabled)
{
    if (str_cmp(feature_name, "vignette") == 0) {
        ::platform->data()->show_vignette_ = enabled;
    }
}


////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


Platform::DeltaClock::DeltaClock() : impl_(new sf::Clock)
{
}


std::chrono::time_point throttle_start =
    std::chrono::high_resolution_clock::now();
std::chrono::time_point throttle_stop =
    std::chrono::high_resolution_clock::now();
static int sleep_time;


Microseconds Platform::DeltaClock::reset()
{
    // NOTE: I originally developed the game on the nintendo gameboy
    // advance. The game was designed to use delta timing, but stuff still seems
    // to break when running on modern hardware, which has a WAY faster clock
    // than the gameboy. So for now, I'm intentionally slowing things
    // down. Throttling the game logic also saves battery life.

    throttle_stop = std::chrono::high_resolution_clock::now();


#if not defined(__linux__) and not defined(_WIN32)
    // Unfortunately, this code seems to make the linux builds
    // really stuttery. Without this enabled, you're likely to see
    // high cpu usage.
    const auto gba_fixed_step = 2000;
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        throttle_stop - throttle_start);

    if (elapsed.count() < gba_fixed_step) {
        std::this_thread::sleep_for(std::chrono::microseconds(
            (gba_fixed_step - 1000) - (elapsed.count() - sleep_time)));
    }
#endif

    auto val = reinterpret_cast<sf::Clock*>(impl_)->restart().asMicroseconds();

    val -= sleep_time;
    sleep_time = 0;

    throttle_start = std::chrono::high_resolution_clock::now();
    ;


    // On the gameboy advance, I was returning a fixed step based on the screen
    // refresh rate, with I thought was 60Hz. But, in fact, the screen refresh
    // rate on the gameboy was 59.59, so we'll have to scale the delta time to
    // get things to run correctly.
    constexpr float scaling_factor = (60.f / 59.59f);

    return val * scaling_factor;
}


Platform::DeltaClock::~DeltaClock()
{
    delete reinterpret_cast<sf::Clock*>(impl_);
}


////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////

// std::mutex event_queue_lock;
std::queue<sf::Event> event_queue;


std::array<sf::Keyboard::Key, static_cast<int>(Key::count)> keymap;


static std::vector<Platform::Keyboard::ControllerInfo> joystick_info;


void Platform::Keyboard::register_controller(const ControllerInfo& info)
{
    joystick_info.push_back(info);
}


void Platform::Keyboard::poll()
{
    // FIXME: Poll is now called from the logic thread, which means that the
    // graphics loop needs to window enqueue events in a synchronized buffer,
    // and then the logic thread will read events from the buffer and process
    // them.

    for (size_t i = 0; i < size_t(Key::count); ++i) {
        prev_[i] = states_[i];
    }

    static bool focused_ = true;

    // std::lock_guard<std::mutex> guard(::event_queue_lock);
    while (not event_queue.empty()) {
        auto event = event_queue.front();
        event_queue.pop();

        switch (event.type) {
        case sf::Event::LostFocus:
            focused_ = false;
            break;

        case sf::Event::GainedFocus:
            focused_ = true;
            break;

        case sf::Event::Closed:
            ::platform->data()->window_.close();
            break;

        case sf::Event::KeyPressed:
            if (not focused_) {
                break;
            }
            for (int keycode = 0; keycode < static_cast<int>(Key::count);
                 ++keycode) {
                if (keymap[keycode] == event.key.code) {
                    states_[keycode] = true;
                }
            }
            break;

        case sf::Event::KeyReleased:
            if (not focused_) {
                break;
            }
            for (int keycode = 0; keycode < static_cast<int>(Key::count);
                 ++keycode) {
                if (keymap[keycode] == event.key.code) {
                    states_[keycode] = false;
                }
            }
            break;

        // FIXME: It may be undefined behavior to handle joystick events on the
        // non-main thread...
        case sf::Event::JoystickConnected: {
            info(*::platform,
                 ("joystick " +
                  std::to_string(event.joystickConnect.joystickId) +
                  " connected")
                     .c_str());
            break;
        }

        case sf::Event::JoystickDisconnected: {
            info(*::platform,
                 ("joystick " +
                  std::to_string(event.joystickConnect.joystickId) +
                  " disconnected")
                     .c_str());
            break;
        }

        case sf::Event::JoystickButtonPressed: {
            const auto ident = sf::Joystick::getIdentification(
                event.joystickButton.joystickId);
            for (auto& info : reversed(joystick_info)) {
                if (info.vendor_id == (int)ident.vendorId and
                    info.product_id == (int)ident.productId) {
                    const int button = event.joystickButton.button;
                    ::info(*::platform,
                           ("joystick button " +
                            std::to_string(event.joystickButton.button))
                               .c_str());
                    if (button == info.action_1_key) {
                        states_[static_cast<int>(Key::action_1)] = true;
                    } else if (button == info.action_2_key) {
                        states_[static_cast<int>(Key::action_2)] = true;
                    } else if (button == info.start_key) {
                        states_[static_cast<int>(Key::start)] = true;
                    } else if (button == info.alt_1_key) {
                        states_[static_cast<int>(Key::alt_1)] = true;
                    } else if (button == info.alt_2_key) {
                        states_[static_cast<int>(Key::alt_2)] = true;
                    }
                    break;
                }
            }
            break;
        }

        case sf::Event::JoystickButtonReleased: {
            const auto ident = sf::Joystick::getIdentification(
                event.joystickButton.joystickId);
            for (auto& info : reversed(joystick_info)) {
                if (info.vendor_id == (int)ident.vendorId and
                    info.product_id == (int)ident.productId) {
                    const int button = event.joystickButton.button;
                    if (button == info.action_1_key) {
                        states_[static_cast<int>(Key::action_1)] = false;
                    } else if (button == info.action_2_key) {
                        states_[static_cast<int>(Key::action_2)] = false;
                    } else if (button == info.start_key) {
                        states_[static_cast<int>(Key::start)] = false;
                    } else if (button == info.alt_1_key) {
                        states_[static_cast<int>(Key::alt_1)] = false;
                    } else if (button == info.alt_2_key) {
                        states_[static_cast<int>(Key::alt_2)] = false;
                    }
                    break;
                }
            }
            break;
        }

        case sf::Event::JoystickMoved: {
            const auto ident = sf::Joystick::getIdentification(
                event.joystickButton.joystickId);
            for (auto& info : reversed(joystick_info)) {
                if (info.vendor_id == (int)ident.vendorId and
                    info.product_id == (int)ident.productId) {
                    static const int deadZone = 30;
                    if (event.joystickMove.axis == sf::Joystick::Axis::X) {
                        float position = event.joystickMove.position;
                        if (position > deadZone) {
                            states_[static_cast<int>(Key::right)] = true;
                            states_[static_cast<int>(Key::left)] = false;
                        } else if (-position > deadZone) {
                            states_[static_cast<int>(Key::left)] = true;
                            states_[static_cast<int>(Key::right)] = false;
                        } else {
                            states_[static_cast<int>(Key::left)] = false;
                            states_[static_cast<int>(Key::right)] = false;
                        }
                    } else if (event.joystickMove.axis ==
                               sf::Joystick::Axis::Y) {
                        float position = event.joystickMove.position;
                        if (-position > deadZone) {
                            states_[static_cast<int>(Key::up)] = true;
                            states_[static_cast<int>(Key::down)] = false;
                        } else if (position > deadZone) {
                            states_[static_cast<int>(Key::down)] = true;
                            states_[static_cast<int>(Key::up)] = false;
                        } else {
                            states_[static_cast<int>(Key::up)] = false;
                            states_[static_cast<int>(Key::down)] = false;
                        }
                    }
                    break;
                }
            }
            break;
        }

        default:
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// Screen
////////////////////////////////////////////////////////////////////////////////


Platform::Screen::Screen()
{
}


void Platform::Screen::enable_night_mode(bool)
{
    // TODO...
}


Vec2<u32> Platform::Screen::size() const
{
    const auto data = ::platform->data();
    return {data->window_size_.x / data->window_scale_,
            data->window_size_.y / data->window_scale_};
}


void Platform::Screen::set_contrast(Contrast c)
{
    // TODO...
}


Contrast Platform::Screen::get_contrast() const
{
    return 0; // TODO...
}


enum class TextureSwap { spritesheet, tile0, tile1, overlay };

// The logic thread requests a texture swap, but the swap itself needs to be
// performed on the graphics thread.
static std::queue<std::pair<TextureSwap, std::string>> texture_swap_requests;
// static std::mutex texture_swap_mutex;


static std::queue<std::tuple<Layer, int, int, int>> tile_swap_requests;
// static std::mutex tile_swap_mutex;


static std::queue<std::pair<TileDesc, Platform::TextureMapping>> glyph_requests;
// static std::mutex glyph_requests_mutex;


static std::vector<Platform::Task*> task_queue;


void Platform::push_task(Task* task)
{
    task->complete_ = false;
    task->running_ = true;

    task_queue.push_back(task);
}


void Platform::Screen::clear()
{
    for (auto it = task_queue.begin(); it not_eq task_queue.end();) {
        (*it)->run();
        if ((*it)->complete()) {
            (*it)->running_ = false;
            it = task_queue.erase(it);
        } else {
            ++it;
        }
    }

    auto& window = ::platform->data()->window_;
    auto& rt = ::platform->data()->rt_;
    window.clear();
    rt.clear();

    ::platform->data()->fade_overlay_.setFillColor(
        ::platform->data()->fade_color_);

    {
        // std::lock_guard<std::mutex> guard(::platform->data()->audio_lock_);
        auto& sounds = ::platform->data()->sounds_;
        for (auto it = sounds.begin(); it not_eq sounds.end();) {
            if (it->getStatus() == sf::Sound::Status::Stopped) {
                it = sounds.erase(it);
            } else {
                ++it;
            }
        }
    }

    {
        // std::lock_guard<std::mutex> guard(texture_swap_mutex);
        while (not texture_swap_requests.empty()) {
            const auto request = texture_swap_requests.front();
            texture_swap_requests.pop();

            sf::Image image;

            auto image_folder = resource_path() + ("images" PATH_DELIMITER);

            if (not image.loadFromFile(image_folder + request.second +
                                       ".png")) {
                error(*::platform,
                      (std::string("failed to load texture ") + request.second)
                          .c_str());
                exit(EXIT_FAILURE);
            } else {
                info(*::platform,
                     (std::string("loaded image ") + request.second).c_str());
            }
            image.createMaskFromColor({255, 0, 255, 255});
            image.saveToFile("/home/evan/blind-jump-portable/build/test." +
                             request.second + ".png");

            // For space savings on the gameboy advance, I used tile0 for the
            // background as well. But it was meta-tiled as 4x3, so we need to
            // create a meta-tiled version of the tile0 for use as the
            // background texture...
            if (request.first == TextureSwap::tile0) {
                // But... we need to support loading a non-standard map texture,
                // for the purpose of displaying images, so do not metatile if
                // the image height is 8 (already metatiled!).
                //
                if (image.getSize().y not_eq 8) {
                    sf::Image meta_image;
                    meta_image.create(image.getSize().x * 3, 8);

                    for (size_t block = 0; block < image.getSize().x / 32;
                         ++block) {
                        for (int row = 0; row < 3; ++row) {
                            const int src_x = block * 32;
                            const int src_y = row * 8;

                            const int dest_x = block * (32 * 3) + row * 32;
                            const int dest_y = 0;

                            meta_image.copy(
                                image, dest_x, dest_y, {src_x, src_y, 32, 8});
                        }
                    }

                    if (not ::platform->data()
                                ->background_texture_.loadFromImage(
                                    meta_image)) {
                        error(*::platform,
                              "Failed to create background texture");
                        exit(EXIT_FAILURE);
                    }

                    if (not platform->data()->tile0_texture_.loadFromImage(
                            image)) {
                        error(*::platform, "Failed to create tile0 texture");
                    }

                } else {
                    if (not ::platform->data()
                                ->background_texture_.loadFromImage(image)) {
                        error(*::platform,
                              "Failed to create background texture");
                        exit(EXIT_FAILURE);
                    }

                    sf::Image unmeta_image;
                    unmeta_image.create(image.getSize().x / 3, 24);
                }

            } else if (not [&] {
                           switch (request.first) {
                           case TextureSwap::spritesheet:
                               return &::platform->data()->spritesheet_texture_;

                           case TextureSwap::tile0:
                               break;

                           case TextureSwap::tile1:
                               return &::platform->data()->tile1_texture_;

                           case TextureSwap::overlay:
                               return &::platform->data()->overlay_texture_;
                           }
                           error(*::platform,
                                 "invalid texture swap enumeration");
                           ::platform->fatal();
                       }()
                               ->loadFromImage(image)) {
                error(*::platform, "Failed to create texture");
                exit(EXIT_FAILURE);
            }
        }
    }


    {
        // std::lock_guard<std::mutex> guard(glyph_requests_mutex);
        while (not glyph_requests.empty()) {
            const auto rq = glyph_requests.front();
            glyph_requests.pop();

            auto image_folder = resource_path() + ("images" PATH_DELIMITER);

            sf::Image character_source_image_;
            const auto charset_path =
                std::string(image_folder) + rq.second.texture_name_ + ".png";
            if (not character_source_image_.loadFromFile(charset_path)) {
                error(
                    *::platform,
                    (std::string("failed to open charset image " + charset_path)
                         .c_str()));
                exit(EXIT_FAILURE);
            }

            // This code is so wasteful... so many intermediary images... FIXME.

            auto& texture = ::platform->data()->overlay_texture_;
            auto old_texture_img = texture.copyToImage();

            sf::Image new_texture_image;
            new_texture_image.create((rq.first + 1) * 8,
                                     old_texture_img.getSize().y);

            new_texture_image.copy(old_texture_img,
                                   0,
                                   0,
                                   {0,
                                    0,
                                    (int)old_texture_img.getSize().x,
                                    (int)old_texture_img.getSize().y});

            new_texture_image.copy(character_source_image_,
                                   rq.first * 8,
                                   0,
                                   {rq.second.offset_ * 8, 0, 8, 8},
                                   true);

            const auto glyph_background_color =
                character_source_image_.getPixel(0, 0);

            const auto font_fg_color = new_texture_image.getPixel(648, 0);
            const auto font_bg_color = new_texture_image.getPixel(649, 0);

            for (int x = 0; x < 8; ++x) {
                for (int y = 0; y < 8; ++y) {
                    const auto px =
                        new_texture_image.getPixel(rq.first * 8 + x, y);
                    if (px == glyph_background_color) {
                        new_texture_image.setPixel(
                            rq.first * 8 + x, y, font_bg_color);
                    } else {
                        new_texture_image.setPixel(
                            rq.first * 8 + x, y, font_fg_color);
                    }
                }
            }

            // character_source_image_.saveToFile("debug.png");
            // new_texture_image.saveToFile("test.png");

            texture.loadFromImage(new_texture_image);
        }
    }

    {
        // std::lock_guard<std::mutex> guard(::tile_swap_mutex);
        while (not tile_swap_requests.empty()) {
            const auto request = tile_swap_requests.front();
            tile_swap_requests.pop();

            switch (std::get<0>(request)) {
            case Layer::overlay:
                ::platform->data()->overlay_.set_tile(std::get<1>(request),
                                                      std::get<2>(request),
                                                      std::get<3>(request));
                break;

            case Layer::map_0:
                ::platform->data()->map_0_changed_ = true;
                ::platform->data()->map_0_.set_tile(std::get<1>(request),
                                                    std::get<2>(request),
                                                    std::get<3>(request));
                break;

            case Layer::map_1:
                ::platform->data()->map_1_changed_ = true;
                ::platform->data()->map_1_.set_tile(std::get<1>(request),
                                                    std::get<2>(request),
                                                    std::get<3>(request));
                break;

            case Layer::background:
                ::platform->data()->background_changed_ = true;
                ::platform->data()->background_.set_tile(std::get<1>(request),
                                                         std::get<2>(request),
                                                         std::get<3>(request));
                break;
            }
        }
    }

    {
        // std::lock_guard<std::mutex> guard(::event_queue_lock);

        // This is really hacky and bad. We can't check for keypresses on the
        // other thread, and the sfml window does not fire keypressed events
        // consistently while a key is held down, so we're generating our own
        // events.
        for (int keycode = 0; keycode < static_cast<int>(Key::count);
             ++keycode) {
            sf::Event event;
            if (sf::Keyboard::isKeyPressed(keymap[keycode])) {
                event.type = sf::Event::KeyPressed;
                event.key.code = keymap[keycode];
            } else {
                event.type = sf::Event::KeyReleased;
                event.key.code = keymap[keycode];
            }
            ::event_queue.push(event);
        }


        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::KeyPressed:
            case sf::Event::KeyReleased:
                // We're generating our own events, see above.
                break;

            default:
                ::event_queue.push(event);
            }
        }
    }
}


std::vector<Sprite> draw_queue;


static sf::Glsl::Vec3 make_color(int color_hex)
{
    const auto r = (color_hex & 0xFF0000) >> 16;
    const auto g = (color_hex & 0x00FF00) >> 8;
    const auto b = (color_hex & 0x0000FF);

    return {r / 255.f, g / 255.f, b / 255.f};
}


static sf::Glsl::Vec3 real_color(ColorConstant k)
{
    return make_color(static_cast<int>(k));
}


sf::View get_letterbox_view(sf::View view, int window_width, int window_height)
{

    // Compares the aspect ratio of the window to the aspect ratio of the view,
    // and sets the view's viewport accordingly in order to archieve a letterbox effect.
    // A new view (with a new viewport set) is returned.

    float window_ratio = window_width / (float)window_height;
    float view_ratio = view.getSize().x / (float)view.getSize().y;
    float size_x = 1;
    float size_y = 1;
    float pos_x = 0;
    float pos_y = 0;

    bool horizontal_spacing = true;
    if (window_ratio < view_ratio)
        horizontal_spacing = false;

    // If horizontalSpacing is true, the black bars will appear on the left and right side.
    // Otherwise, the black bars will appear on the top and bottom.

    if (horizontal_spacing) {
        size_x = view_ratio / window_ratio;
        pos_x = (1 - size_x) / 2.f;
    }

    else {
        size_y = window_ratio / view_ratio;
        pos_y = (1 - size_y) / 2.f;
    }

    view.setViewport(sf::FloatRect(pos_x, pos_y, size_x, size_y));

    return view;
}


void Platform::Screen::display()
{
    sf::View view;
    view.setSize(view_.get_size().x, view_.get_size().y);

    auto& window = ::platform->data()->window_;
    auto& rt = ::platform->data()->rt_;

    if (::platform->data()->background_changed_) {
        ::platform->data()->background_changed_ = false;
        ::platform->data()->background_rt_.clear(sf::Color::Transparent);
        ::platform->data()->background_rt_.draw(
            ::platform->data()->background_);
        ::platform->data()->background_rt_.display();
    }

    {
        view.setCenter(view_.get_center().x * 0.3f + view_.get_size().x / 2,
                       view_.get_center().y * 0.3f + view_.get_size().y / 2);
        rt.setView(view);

        sf::Sprite bkg_spr(::platform->data()->background_rt_.getTexture());

        rt.draw(bkg_spr);

        const auto right_x_wrap_threshold = 51.f;
        const auto bottom_y_wrap_threshold = 320.f;

        // Manually wrap the background sprite
        if (view_.get_center().x > right_x_wrap_threshold) {
            bkg_spr.setPosition(256, 0);
            rt.draw(bkg_spr);
        }

        if (view_.get_center().x < 0.f) {
            bkg_spr.setPosition(-256, 0);
            rt.draw(bkg_spr);
        }

        if (view_.get_center().y < 0.f) {
            bkg_spr.setPosition(0, -256);
            rt.draw(bkg_spr);

            if (view_.get_center().x < 0.f) {
                bkg_spr.setPosition(-256, -256);
                rt.draw(bkg_spr);
            } else if (view_.get_center().x > right_x_wrap_threshold) {
                bkg_spr.setPosition(256, -256);
                rt.draw(bkg_spr);
            }
        }

        if (view_.get_center().y > bottom_y_wrap_threshold) {
            bkg_spr.setPosition(0, 256);
            rt.draw(bkg_spr);

            if (view_.get_center().x < 0.f) {
                bkg_spr.setPosition(-256, 256);
                rt.draw(bkg_spr);
            } else if (view_.get_center().x > right_x_wrap_threshold) {
                bkg_spr.setPosition(256, 256);
                rt.draw(bkg_spr);
            }
        }
    }

    view.setCenter(view_.get_center().x + view_.get_size().x / 2,
                   view_.get_center().y + view_.get_size().y / 2);
    rt.setView(view);

    if (::platform->data()->map_0_changed_) {
        ::platform->data()->map_0_changed_ = false;
        ::platform->data()->map_0_rt_.clear(sf::Color::Transparent);
        ::platform->data()->map_0_rt_.draw(::platform->data()->map_0_);
        ::platform->data()->map_0_rt_.display();
    }

    if (::platform->data()->map_1_changed_) {
        ::platform->data()->map_1_changed_ = false;
        ::platform->data()->map_1_rt_.clear(sf::Color::Transparent);
        ::platform->data()->map_1_rt_.draw(::platform->data()->map_1_);
        ::platform->data()->map_1_rt_.display();
    }

    rt.draw(sf::Sprite(::platform->data()->map_0_rt_.getTexture()));
    rt.draw(sf::Sprite(::platform->data()->map_1_rt_.getTexture()));

    ::platform->data()->fade_overlay_.setPosition(
        {view_.get_center().x, view_.get_center().y});

    const bool fade_sprites = ::platform->data()->fade_include_sprites_;
    const bool fade_overlay = ::platform->data()->fade_include_overlay_;

    // If we don't want the sprites to be included in the color fade, we'll want
    // to draw the fade overlay prior to drawing the sprites... or we could quit
    // being lazy and use a shader instead of a dumb rectangleshape :)
    if (not fade_sprites) {
        rt.draw(::platform->data()->fade_overlay_);
    }

    for (auto& spr : reversed(::draw_queue)) {
        if (spr.get_alpha() == Sprite::Alpha::transparent) {
            continue;
        }
        const Vec2<Float>& pos = spr.get_position();
        const Vec2<bool>& flip = spr.get_flip();

        sf::Sprite sf_spr;

        if (auto rot = spr.get_rotation()) {
            sf_spr.setRotation((float(rot) / std::numeric_limits<s16>::max()) *
                               360);
        }

        if (spr.get_scale().x or spr.get_scale().y) {
            Float x_scale = 1;
            Float y_scale = 1;
            if (spr.get_scale().x < 0) {
            }
            if (spr.get_scale().y < 0) {
            }

            sf_spr.setScale(x_scale, y_scale);
        }

        sf_spr.setPosition({pos.x, pos.y});
        sf_spr.setOrigin(
            {float(spr.get_origin().x), float(spr.get_origin().y)});
        if (spr.get_alpha() == Sprite::Alpha::translucent) {
            sf_spr.setColor({255, 255, 255, 128});
        }

        sf_spr.setScale({flip.x ? -1.f : 1.f, flip.y ? -1.f : 1.f});

        sf_spr.setTexture(::platform->data()->spritesheet_texture_);

        switch (const auto ind = spr.get_texture_index(); spr.get_size()) {
        case Sprite::Size::w16_h32:
            sf_spr.setTextureRect({static_cast<s32>(ind) * 16, 0, 16, 32});
            break;

        case Sprite::Size::w32_h32:
            sf_spr.setTextureRect({static_cast<s32>(ind) * 32, 0, 32, 32});
            break;
        }

        if (const auto& mix = spr.get_mix();
            mix.color_ not_eq ColorConstant::null) {
            sf::Shader& shader = ::platform->data()->color_shader_;
            shader.setUniform("amount", mix.amount_ / 255.f);
            shader.setUniform("targetColor", real_color(mix.color_));
            rt.draw(sf_spr, &shader);
        } else {
            rt.draw(sf_spr);
        }
    }

    const auto cached_view = view;
    if (fade_sprites and not fade_overlay) {
        rt.draw(::platform->data()->fade_overlay_);
    }

    auto& origin = ::platform->data()->overlay_origin_;

    view.setCenter(
        {view_.get_size().x / 2 + origin.x, view_.get_size().y / 2 + origin.y});

    rt.setView(view);

    if (::platform->data()->show_vignette_) {
        sf::Sprite vignette(::platform->data()->vignette_texture_);
        vignette.setScale(240.f / 450.f, 160.f / 450.f);
        vignette.setColor(sf::Color(255, 255, 255, 255));
        rt.draw(vignette, sf::BlendMultiply);
    }

    rt.draw(::platform->data()->overlay_);

    if (fade_overlay) {
        rt.setView(cached_view);
        rt.draw(::platform->data()->fade_overlay_);
    }

    rt.display();

    view.setSize(view_.get_size().x, view_.get_size().y);

    view = get_letterbox_view(view,
                              ::platform->data()->window_.getSize().x,
                              ::platform->data()->window_.getSize().y);

    window.setView(view);

    window.draw(sf::Sprite(rt.getTexture()));

    window.display();

    draw_queue.clear();
}


void Platform::Screen::fade(Float amount,
                            ColorConstant k,
                            std::optional<ColorConstant> base,
                            bool include_sprites,
                            bool include_overlay)
{
    const auto c = real_color(k);

    if (not base) {
        ::platform->data()->fade_color_ = {static_cast<uint8_t>(c.x * 255),
                                           static_cast<uint8_t>(c.y * 255),
                                           static_cast<uint8_t>(c.z * 255),
                                           static_cast<uint8_t>(amount * 255)};
        ::platform->data()->fade_include_sprites_ = include_sprites;
        ::platform->data()->fade_include_overlay_ = include_overlay;
    } else {
        const auto c2 = real_color(*base);
        ::platform->data()->fade_color_ = {
            interpolate(static_cast<uint8_t>(c.x * 255),
                        static_cast<uint8_t>(c2.x * 255),
                        amount),
            interpolate(static_cast<uint8_t>(c.y * 255),
                        static_cast<uint8_t>(c2.y * 255),
                        amount),
            interpolate(static_cast<uint8_t>(c.z * 255),
                        static_cast<uint8_t>(c2.z * 255),
                        amount),
            static_cast<uint8_t>(255)};
    }
}


void Platform::Screen::pixelate(u8 amount,
                                bool include_overlay,
                                bool include_background,
                                bool include_sprites)
{
    // TODO... Need to work on the shader pipeline...
}


void Platform::Screen::draw(const Sprite& spr)
{
    draw_queue.push_back(spr);
}


////////////////////////////////////////////////////////////////////////////////
// Speaker
////////////////////////////////////////////////////////////////////////////////


void Platform::Speaker::set_position(const Vec2<Float>& position)
{
    // sf::Listener::setPosition({position.x, 0, position.y});
}


Platform::Speaker::Speaker()
{
    // TODO...
}


void Platform::Speaker::play_music(const char* name, Microseconds offset)
{
    std::string path = resource_path() + ("sounds" PATH_DELIMITER);

    path += "music_";
    path += name;
    path += ".ogg";

    if (::platform->data()->music_.openFromFile(path.c_str())) {
        ::platform->data()->music_.play();
        ::platform->data()->music_.setLoop(true);
        ::platform->data()->music_.setPlayingOffset(sf::microseconds(offset));
        // For desktop releases, we have spatialized sound enabled, such that
        // the sound effects attenuate based on their distance from the center
        // of the screen. The music seems too loud by comparison, so I'm
        // playing the music more quietly than I would on the gameboy advance,
        // where everthing is mono (there's only one speaker on the device, so
        // stereo isn't really worth the resources on the gameboy).
        ::platform->data()->music_.setVolume(70);
    } else {
        error(*::platform, "failed to load music file");
    }
}


void Platform::Speaker::stop_music()
{
    ::platform->data()->music_.stop();
}


void Platform::Speaker::play_sound(const char* name,
                                   int priority,
                                   std::optional<Vec2<Float>> position)
{
    (void)priority; // We are not using the priority variable, because we're a
                    // powerful desktop pc, we can play lots of sounds at once,
                    // and do not need to evict sounds! Or do we...? I think
                    // SFML does have a hard upper limit of 256 concurrent audio
                    // streams, but we'll probably never hit that limit, given
                    // that the game was originally calibrated for the gameboy
                    // advance, where I was supporting four concurrent audio
                    // streams.

    // std::lock_guard<std::mutex> guard(::platform->data()->audio_lock_);
    auto& data = ::platform->data()->sound_data_;
    auto found = data.find(name);
    if (found not_eq data.end()) {
        ::platform->data()->sounds_.emplace_back(found->second);

        auto& sound = ::platform->data()->sounds_.back();

        sound.play();

        // sound.setRelativeToListener(static_cast<bool>(position));

        // if (position) {
        //     sound.setAttenuation(0.2f);
        // } else {
        //     sound.setAttenuation(0.f);
        // }

        // sound.setMinDistance(160.f);
        // if (position) {
        //     sound.setPosition({position->x, 0, position->y});
        // }
    } else {
        error(*::platform, (std::string("no sound data for ") + name).c_str());
    }
}


bool is_sound_playing(const char* name)
{
    return false; // TODO...
}


////////////////////////////////////////////////////////////////////////////////
// RemoteConsole
////////////////////////////////////////////////////////////////////////////////


bool Platform::RemoteConsole::supported_by_device()
{
    return true;
}


bool Platform::RemoteConsole::readline(bool (*callback)(Platform&, const char*))
{
    std::string line;
    auto result = static_cast<bool>(std::getline(std::cin, line));
    if (result) {
        return callback(*::platform, line.c_str());
    }
    return result;
}


void Platform::RemoteConsole::print(const char* text)
{
    std::cout << text;
}


////////////////////////////////////////////////////////////////////////////////
// Logger
////////////////////////////////////////////////////////////////////////////////


static const char* const logfile_name = "logfile.txt";
static std::ofstream logfile_out(logfile_name);


static Severity log_threshold;


void Platform::Logger::set_threshold(Severity severity)
{
    log_threshold = severity;
}


void Platform::Logger::log(Severity level, const char* msg)
{
    if (static_cast<int>(level) < static_cast<int>(::log_threshold)) {
        return;
    }

    auto write_msg = [&](std::ostream& target) {
        target << '[' <<
            [&] {
                switch (level) {
                default:
                case Severity::info:
                    return "info";
                case Severity::warning:
                    return "warning";
                case Severity::error:
                    return "error";
                }
            }() << "] "
               << msg << '\n';

        if (level == Severity::error or level == Severity::warning) {
            target << std::flush;
        }
    };

    write_msg(logfile_out);
    write_msg(std::cout);
}


void Platform::Logger::read(void* buffer, u32 start_offset, u32 num_bytes)
{
    const std::string data = [&] {
        std::ifstream logfile_in(logfile_name);
        std::stringstream strbuf;
        strbuf << logfile_in.rdbuf();
        return strbuf.str();
    }();

    if (int(data.size() - start_offset) < int(num_bytes)) {
        return;
    }

    for (u32 i = start_offset; i < start_offset + num_bytes; ++i) {
        ((char*)buffer)[i - start_offset] = data[i];
    }
}


Platform::Logger::Logger()
{
}


////////////////////////////////////////////////////////////////////////////////
// Platform
////////////////////////////////////////////////////////////////////////////////


static const std::unordered_map<std::string, sf::Keyboard::Key> key_lookup{
    {"Esc", sf::Keyboard::Escape},  {"Up", sf::Keyboard::Up},
    {"Down", sf::Keyboard::Down},   {"Left", sf::Keyboard::Left},
    {"Right", sf::Keyboard::Right}, {"Return", sf::Keyboard::Return},
    {"A", sf::Keyboard::A},         {"B", sf::Keyboard::B},
    {"C", sf::Keyboard::C},         {"D", sf::Keyboard::D},
    {"E", sf::Keyboard::E},         {"F", sf::Keyboard::F},
    {"G", sf::Keyboard::G},         {"H", sf::Keyboard::H},
    {"I", sf::Keyboard::I},         {"J", sf::Keyboard::J},
    {"K", sf::Keyboard::K},         {"L", sf::Keyboard::L},
    {"M", sf::Keyboard::M},         {"N", sf::Keyboard::N},
    {"O", sf::Keyboard::O},         {"P", sf::Keyboard::P},
    {"Q", sf::Keyboard::Q},         {"R", sf::Keyboard::R},
    {"S", sf::Keyboard::S},         {"T", sf::Keyboard::T},
    {"U", sf::Keyboard::U},         {"V", sf::Keyboard::V},
    {"W", sf::Keyboard::W},         {"X", sf::Keyboard::X},
    {"Y", sf::Keyboard::Y},         {"Z", sf::Keyboard::Z}};


Platform::~Platform()
{
    delete data_;
}


// EWRAM is large, but has a narrower bus. The platform offers a window into
// EWRAM, called scratch space, for non-essential stuff. Right now, I am setting
// the buffer to ~100K in size. One could theoretically make the buffer almost
// 256kB, because I am using none of EWRAM as far as I know...
static ObjectPool<RcBase<Platform::ScratchBuffer, 100>::ControlBlock, 100>
    scratch_buffer_pool;


static int scratch_buffers_in_use = 0;


Rc<Platform::ScratchBuffer, 100> Platform::make_scratch_buffer()
{
    auto finalizer =
        [](RcBase<Platform::ScratchBuffer, 100>::ControlBlock* ctrl) {
            --scratch_buffers_in_use;
            ctrl->pool_->post(ctrl);
        };

    auto maybe_buffer =
        Rc<ScratchBuffer, 100>::create(&scratch_buffer_pool, finalizer);
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
    return 100 - scratch_buffers_in_use;
}


static std::optional<DateTime> start_time;


std::optional<DateTime> Platform::startup_time() const
{
    return ::start_time;
}


Platform::Platform()
{
    ::platform = this;

    // push_task(&::watchdog_task);

    data_ = new Data(*this);
    if (not data_) {
        error(*this, "Failed to allocate context");
        exit(EXIT_FAILURE);
    }


    ::start_time = system_clock_.now();


    screen_.view_.set_size(screen_.size().cast<Float>());


    auto shader_folder = resource_path() + ("shaders" PATH_DELIMITER);
    // lisp::loadv<lisp::Symbol>("shader-dir").name_;


    if (not data_->color_shader_.loadFromFile(
            shader_folder + std::string("colorShader.frag"),
            sf::Shader::Fragment)) {
        error(*this, "Failed to load shader");
    }
    data_->color_shader_.setUniform("texture", sf::Shader::CurrentTexture);

    data_->fade_overlay_.setSize(
        {Float(screen_.size().x), Float(screen_.size().y)});


    data_->map_0_rt_.create(16 * 32, 20 * 24);
    data_->map_1_rt_.create(16 * 32, 20 * 24);
    data_->background_rt_.create(32 * 8, 32 * 8);

    keymap[(int)Key::left] = sf::Keyboard::Left;
    keymap[(int)Key::right] = sf::Keyboard::Right;
    keymap[(int)Key::up] = sf::Keyboard::Up;
    keymap[(int)Key::down] = sf::Keyboard::Down;
    keymap[(int)Key::action_1] = sf::Keyboard::X;
    keymap[(int)Key::action_2] = sf::Keyboard::Z;
    keymap[(int)Key::alt_1] = sf::Keyboard::A;
    keymap[(int)Key::alt_2] = sf::Keyboard::S;
    keymap[(int)Key::start] = sf::Keyboard::Return;
    keymap[(int)Key::select] = sf::Keyboard::Q;
}


void Platform::soft_exit()
{
    data()->window_.close();
}


static const char* const save_file_name = "save.dat";


bool Platform::write_save_data(const void* data, u32 length)
{
    std::ofstream out(save_file_name,
                      std::ios_base::out | std::ios_base::binary);

    out.write(reinterpret_cast<const char*>(data), length);

    return true;
}


bool Platform::read_save_data(void* buffer, u32 data_length)
{
    std::ifstream in(save_file_name, std::ios_base::in | std::ios_base::binary);

    if (!in) {
        return false;
    }

    in.read(reinterpret_cast<char*>(buffer), data_length);

    return true;
}


bool Platform::is_running() const
{
    return data()->window_.isOpen();
}


void Platform::sleep(u32 frames)
{
    const auto amount =
        frames * (std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::seconds(1)) /
                  60);

    std::this_thread::sleep_for(amount);

    sleep_time += amount.count();
}


void Platform::load_sprite_texture(const char* name)
{
    // std::lock_guard<std::mutex> guard(texture_swap_mutex);
    texture_swap_requests.push({TextureSwap::spritesheet, name});
}


void Platform::load_tile0_texture(const char* name)
{
    // std::lock_guard<std::mutex> guard(texture_swap_mutex);
    texture_swap_requests.push({TextureSwap::tile0, name});
}


void Platform::load_tile1_texture(const char* name)
{
    // std::lock_guard<std::mutex> guard(texture_swap_mutex);
    texture_swap_requests.push({TextureSwap::tile1, name});
}


void Platform::load_overlay_texture(const char* name)
{
    {
        // std::lock_guard<std::mutex> guard(texture_swap_mutex);
        texture_swap_requests.push({TextureSwap::overlay, name});
    }
    {
        // std::lock_guard<std::mutex> guard(glyph_requests_mutex);
        while (not glyph_requests.empty())
            glyph_requests.pop();
    }
    ::platform->data()->glyph_table_.clear();
    ::platform->data()->next_glyph_ = glyph_region_start;
}


void Platform::on_watchdog_timeout(WatchdogCallback callback)
{
    // TODO
}


std::map<Layer, std::map<std::pair<u16, u16>, TileDesc>> tile_layers_;


void Platform::set_tile(Layer layer, u16 x, u16 y, TileDesc val)
{
    tile_layers_[layer][{x, y}] = val;

    // std::lock_guard<std::mutex> guard(::tile_swap_mutex);
    ::tile_swap_requests.push({layer, x, y, val});
}


void Platform::set_tile(u16 x, u16 y, TileDesc glyph, const FontColors& colors)
{
    // FIXME: implement custom text colors!

    set_tile(Layer::overlay, x, y, glyph);
}


TileDesc Platform::get_tile(Layer layer, u16 x, u16 y)
{
    return tile_layers_[layer][{x, y}];
}


void Platform::fill_overlay(u16 tile_desc)
{
    for (auto& kvp : tile_layers_[Layer::overlay]) {
        kvp.second = tile_desc;
    }

    // std::lock_guard<std::mutex> guard(::tile_swap_mutex);

    for (int i = 0; i < ::platform->data()->overlay_.size().x; ++i) {
        for (int j = 0; j < ::platform->data()->overlay_.size().y; ++j) {
            ::tile_swap_requests.push({Layer::overlay, i, j, tile_desc});
        }
    }
}


void Platform::set_overlay_origin(Float x, Float y)
{
    ::platform->data()->overlay_origin_ = {x, y};
}


void Platform::enable_glyph_mode(bool enabled)
{
    // TODO
}


TileDesc Platform::map_glyph(const utf8::Codepoint& glyph,
                             TextureCpMapper mapper)
{
    auto& glyphs = ::platform->data()->glyph_table_;

    if (auto mapping = mapper(glyph)) {

        auto found = glyphs.find(glyph);
        if (found not_eq glyphs.end()) {
            return found->second;
        } else {
            const auto loc = ::platform->data()->next_glyph_++;
            glyphs[glyph] = loc;

            // std::lock_guard<std::mutex> guard(glyph_requests_mutex);
            glyph_requests.push({loc, *mapping});

            return loc;
        }
    } else {
        return 111;
    }
}


void Platform::fatal()
{
    exit(1);
}


void Platform::feed_watchdog()
{
    ::watchdog_task.feed();
}


static std::map<std::string, std::string> scripts;


const char* Platform::load_file_contents(const char* folder,
                                         const char* filename) const
{
    const auto name = std::string(folder) + PATH_DELIMITER + filename;
    const auto found = scripts.find(name);
    if (found == scripts.end()) {
        std::fstream file(resource_path() + name);
        std::stringstream buffer;
        buffer << file.rdbuf();
        scripts[name] = buffer.str();
    } else {
        return found->second.c_str();
    }

    return scripts[name].c_str();
}


void start(Platform&);


int argc = 0;
char** argv = nullptr;


int main(int argc, char** argv)
{
    rng::critical_state = time(nullptr);

    ::argc = argc;
    ::argv = argv;

    Platform pf;
    start(pf);
}


const char* Platform::get_opt(char opt)
{
    try {
        popl::OptionParser op("Allowed options");
        auto help_option =
            op.add<popl::Switch>("h", "help", "produce help message");
        auto eval_option =
            op.add<popl::Value<std::string>>("e", "eval", "evaluate lisp");

        op.parse(::argc, ::argv);

        switch (opt) {
        case 'e':
            if (eval_option->is_set()) {
                static std::string eval_result = eval_option->value();
                return eval_result.c_str();
            }
            break;
        }
    } catch (...) {
        // ... TODO ...
    }
    return nullptr;
}


void SynchronizedBase::init(Platform& pf)
{
    // impl_ = new std::mutex;
    if (not impl_) {
        error(pf, "failed to allocate mutex");
    }
}


void SynchronizedBase::lock()
{
    // reinterpret_cast<std::mutex*>(impl_)->lock();
}


void SynchronizedBase::unlock()
{
    // reinterpret_cast<std::mutex*>(impl_)->unlock();
}


SynchronizedBase::~SynchronizedBase()
{
    // delete reinterpret_cast<std::mutex*>(impl_);
}


////////////////////////////////////////////////////////////////////////////////
// NetworkPeer
////////////////////////////////////////////////////////////////////////////////


struct NetworkPeerImpl {
    sf::TcpSocket socket_;
    sf::TcpListener listener_;
    bool is_host_ = false;
    int poll_consume_position_ = 0;
};


Platform::NetworkPeer::NetworkPeer() : impl_(nullptr)
{
    auto impl = new NetworkPeerImpl;

    impl->socket_.setBlocking(true);

    impl_ = impl;
}


void Platform::NetworkPeer::disconnect()
{
    auto impl = (NetworkPeerImpl*)impl_;
    impl->socket_.disconnect();

    if (is_connected()) {
        error(*::platform, "disconnect failed?!");
    }
}


bool Platform::NetworkPeer::is_host() const
{
    return ((NetworkPeerImpl*)impl_)->is_host_;
}


bool Platform::NetworkPeer::supported_by_device()
{
    return true;
}


void Platform::NetworkPeer::listen()
{
    auto impl = (NetworkPeerImpl*)impl_;

    auto port = lisp::loadv<lisp::Integer>("network-port").value_;

    info(*::platform, ("listening on port " + std::to_string(port)).c_str());

    impl->listener_.listen(55001);
    impl->listener_.accept(impl->socket_);

    info(*::platform, "Peer connected!");

    impl->is_host_ = true;
    impl->socket_.setBlocking(false);
}


void Platform::NetworkPeer::connect(const char* peer)
{
    auto impl = (NetworkPeerImpl*)impl_;

    impl->is_host_ = false;

    auto port = lisp::loadv<lisp::Integer>("network-port").value_;

    auto addr = "127.0.0.1";
    // Conf{*::platform}.expect<Conf::String>(
    // ::platform->device_name().c_str(), "host_address");

    info(*::platform,
         ("connecting to " + std::string(addr) + ":" + std::to_string(port))
             .c_str());

    if (impl->socket_.connect(addr, port) == sf::Socket::Status::Done) {
        info(*::platform, "Peer connected!");
    } else {
        error(*::platform, "connection failed :(");
    }

    impl->socket_.setBlocking(false);
}


bool Platform::NetworkPeer::is_connected() const
{
    auto impl = (NetworkPeerImpl*)impl_;
    return impl->socket_.getRemoteAddress() not_eq sf::IpAddress::None;
}


bool Platform::NetworkPeer::send_message(const Message& message)
{
    auto impl = (NetworkPeerImpl*)impl_;

    std::size_t sent = 0;
    impl->socket_.send(message.data_, message.length_, sent);

    if (sent not_eq message.length_) {
        warning(*::platform, "part of message not sent!");
        return false;
    }

    return true;
}


static std::vector<u8> receive_buffer;


void Platform::NetworkPeer::update()
{
    auto impl = (NetworkPeerImpl*)impl_;

    if (impl->poll_consume_position_) {
        receive_buffer.erase(receive_buffer.begin(),
                             receive_buffer.begin() +
                                 impl->poll_consume_position_);
    }

    impl->poll_consume_position_ = 0;

    std::array<char, 1024> buffer = {0};
    std::size_t received = 0;

    while (true) {
        impl->socket_.receive(buffer.data(), sizeof buffer, received);

        if (received > 0) {
            std::copy(buffer.begin(),
                      buffer.begin() + received,
                      std::back_inserter(receive_buffer));
        }

        if (received < sizeof buffer) {
            break;
        }
    }
}


std::optional<Platform::NetworkPeer::Message>
Platform::NetworkPeer::poll_message()
{
    auto impl = (NetworkPeerImpl*)impl_;

    if (receive_buffer.empty()) {
        return {};
    }

    if (impl->poll_consume_position_ >= (int)receive_buffer.size()) {
        return {};
    }

    return Message{(byte*)&receive_buffer[impl->poll_consume_position_],
                   (u32)receive_buffer.size() - impl->poll_consume_position_};
}


void Platform::NetworkPeer::poll_consume(u32 length)
{
    auto impl = (NetworkPeerImpl*)impl_;
    impl->poll_consume_position_ += length;
}


Platform::NetworkPeer::~NetworkPeer()
{
    delete (NetworkPeerImpl*)impl_;
}


Platform::NetworkPeer::Stats Platform::NetworkPeer::stats()
{
    return {0, 0, 0, 0, 0};
}


Platform::NetworkPeer::Interface Platform::NetworkPeer::interface() const
{
    return Interface::internet;
}


////////////////////////////////////////////////////////////////////////////////
// SystemClock
////////////////////////////////////////////////////////////////////////////////


std::optional<DateTime> Platform::SystemClock::now()
{
    const auto t = std::chrono::system_clock::now();

    auto tt = std::chrono::system_clock::to_time_t(t);

    tm local_tm = *localtime(&tt);

    DateTime info;
    info.date_.year_ = local_tm.tm_year;
    info.date_.month_ = local_tm.tm_mon;
    info.date_.day_ = local_tm.tm_mday;
    info.hour_ = local_tm.tm_hour;
    info.minute_ = local_tm.tm_min;
    info.second_ = local_tm.tm_sec;

    return info;
}


void Platform::SystemClock::init(Platform& pfrm)
{
}


Platform::SystemClock::SystemClock()
{
}


#ifdef _WIN32
// I am not interested in pulling in all of the stuff in windows.h, some of which is truely horrendous code.A
#ifndef _WINDEF_
class HINSTANCE__;
typedef HINSTANCE__* HINSTANCE;
typedef char* LPSTR;
#endif
int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main();
}
#endif
