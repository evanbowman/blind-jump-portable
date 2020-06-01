#include "conf.hpp"
#include "number/random.hpp"
#include "platform.hpp"


////////////////////////////////////////////////////////////////////////////////
//
//
// Desktop Platform
//
//
////////////////////////////////////////////////////////////////////////////////


#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <iostream>


Platform::DeviceName Platform::device_name() const
{
    return "DesktopPC";
}


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


class Platform::Data {
public:
    sf::Texture spritesheet_texture_;
    sf::Texture tile0_texture_;
    sf::Texture tile1_texture_;
    sf::Texture overlay_texture_;
    sf::Texture background_texture_;
    sf::Shader color_shader_;
    sf::RectangleShape fade_overlay_;

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

    int window_scale_ = 2;
    sf::RenderWindow window_;


    Data(Platform& pfrm)
        : overlay_(&overlay_texture_, {8, 8}, 32, 32),
          map_0_(&tile0_texture_, {32, 24}, 16, 20),
          map_1_(&tile1_texture_, {32, 24}, 16, 20),
          background_(&background_texture_, {8, 8}, 32, 32),
          window_scale_(
              Conf(pfrm).expect<Conf::Integer>(pfrm.device_name().c_str(),
                                               "window_scale")),
          window_(sf::VideoMode(240 * window_scale_, 160 * window_scale_),
                  "Blind Jump",
                  sf::Style::Titlebar | sf::Style::Close)
    {
        window_.setVerticalSyncEnabled(true);
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
        if (::platform->data() and not::platform->is_running()) {
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


////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


DeltaClock::DeltaClock() : impl_(new sf::Clock)
{
}


std::chrono::time_point throttle_start =
    std::chrono::high_resolution_clock::now();
std::chrono::time_point throttle_stop =
    std::chrono::high_resolution_clock::now();
static int sleep_time;


Microseconds DeltaClock::reset()
{
    // NOTE: I originally developed the game on the nintendo gameboy
    // advance. The game was designed to use delta timing, but stuff still seems
    // to break when running on modern hardware, which has a WAY faster clock
    // than the gameboy. So for now, I'm intentionally slowing things
    // down. Throttling the game logic also saves battery life.

    throttle_stop = std::chrono::high_resolution_clock::now();

    const auto gba_fixed_step = 2000;

    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        throttle_stop - throttle_start);
    if (elapsed.count() < gba_fixed_step) {
        std::this_thread::sleep_for(std::chrono::microseconds(
            (gba_fixed_step - 1000) - (elapsed.count() - sleep_time)));
    }

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


DeltaClock::~DeltaClock()
{
    delete reinterpret_cast<sf::Clock*>(impl_);
}


////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////

std::mutex event_queue_lock;
std::queue<sf::Event> event_queue;


std::array<sf::Keyboard::Key, static_cast<int>(Key::count)> keymap;


void Platform::Keyboard::poll()
{
    // FIXME: Poll is now called from the logic thread, which means that the
    // graphics loop needs to window enqueue events in a synchronized buffer,
    // and then the logic thread will read events from the buffer and process
    // them.

    for (size_t i = 0; i < size_t(Key::count); ++i) {
        prev_[i] = states_[i];
    }

    std::lock_guard<std::mutex> guard(::event_queue_lock);
    while (not event_queue.empty()) {
        auto event = event_queue.front();
        event_queue.pop();

        switch (event.type) {
        case sf::Event::Closed:
            ::platform->data()->window_.close();
            break;

        case sf::Event::KeyPressed:
            for (int keycode = 0; keycode < static_cast<int>(Key::count);
                 ++keycode) {
                if (keymap[keycode] == event.key.code) {
                    states_[keycode] = true;
                }
            }
            break;

        case sf::Event::KeyReleased:
            for (int keycode = 0; keycode < static_cast<int>(Key::count);
                 ++keycode) {
                if (keymap[keycode] == event.key.code) {
                    states_[keycode] = false;
                }
            }
            break;

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


Vec2<u32> Platform::Screen::size() const
{
    const auto data = ::platform->data();
    return {data->window_.getSize().x / data->window_scale_,
            data->window_.getSize().y / data->window_scale_};
}


enum class TextureSwap { spritesheet, tile0, tile1, overlay };

// The logic thread requests a texture swap, but the swap itself needs to be
// performed on the graphics thread.
static std::queue<std::pair<TextureSwap, std::string>> texture_swap_requests;
static std::mutex texture_swap_mutex;


static std::queue<std::tuple<Layer, int, int, int>> tile_swap_requests;
static std::mutex tile_swap_mutex;


void Platform::Screen::clear()
{
    auto& window = ::platform->data()->window_;
    window.clear();

    {
        std::lock_guard<std::mutex> guard(texture_swap_mutex);
        while (not texture_swap_requests.empty()) {
            const auto request = texture_swap_requests.front();
            texture_swap_requests.pop();

            sf::Image image;

            auto image_folder =
                Conf(*::platform).expect<Conf::String>("paths", "image_folder");

            if (not image.loadFromFile(image_folder.c_str() + request.second +
                                       ".png")) {
                error(*::platform,
                      (std::string("failed to load texture ") + request.second)
                          .c_str());
                exit(EXIT_FAILURE);
            }
            image.createMaskFromColor({255, 0, 255, 255});

            // For space savings on the gameboy advance, I used tile0 for the
            // background as well. But it was meta-tiled as 4x3, so we need to
            // create a meta-tiled version of the tile0 for use as the
            // background texture...
            if (request.first == TextureSwap::tile0) {
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

                if (not::platform->data()->background_texture_.loadFromImage(
                        meta_image)) {
                    error(*::platform, "Failed to create background texture");
                    exit(EXIT_FAILURE);
                }
                // meta_image.saveToFile("/Users/evanbowman/Desktop/BlindJump/build/meta-test.png");
            }

            if (not[&] {
                    switch (request.first) {
                    case TextureSwap::spritesheet:
                        return &::platform->data()->spritesheet_texture_;

                    case TextureSwap::tile0:
                        return &::platform->data()->tile0_texture_;

                    case TextureSwap::tile1:
                        return &::platform->data()->tile1_texture_;

                    case TextureSwap::overlay:
                        return &::platform->data()->overlay_texture_;
                    }
                }()
                       ->loadFromImage(image)) {
                error(*::platform, "Failed to create texture");
                exit(EXIT_FAILURE);
            }
        }
    }

    {
        std::lock_guard<std::mutex> guard(::tile_swap_mutex);
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
        std::lock_guard<std::mutex> guard(::event_queue_lock);

        sf::Event event;
        while (window.pollEvent(event)) {
            ::event_queue.push(event);
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


void Platform::Screen::display()
{
    sf::View view;
    view.setSize(view_.get_size().x, view_.get_size().y);

    auto& window = ::platform->data()->window_;

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
        window.setView(view);

        sf::Sprite bkg_spr(::platform->data()->background_rt_.getTexture());

        window.draw(bkg_spr);

        const auto right_x_wrap_threshold = 51.f;
        const auto bottom_y_wrap_threshold = 320.f;

        // Manually wrap the background sprite
        if (view_.get_center().x > right_x_wrap_threshold) {
            bkg_spr.setPosition(256, 0);
            window.draw(bkg_spr);
        }

        if (view_.get_center().x < 0.f) {
            bkg_spr.setPosition(-256, 0);
            window.draw(bkg_spr);
        }

        if (view_.get_center().y < 0.f) {
            bkg_spr.setPosition(0, -256);
            window.draw(bkg_spr);

            if (view_.get_center().x < 0.f) {
                bkg_spr.setPosition(-256, -256);
                window.draw(bkg_spr);
            } else if (view_.get_center().x > right_x_wrap_threshold) {
                bkg_spr.setPosition(256, -256);
                window.draw(bkg_spr);
            }
        }

        if (view_.get_center().y > bottom_y_wrap_threshold) {
            bkg_spr.setPosition(0, 256);
            window.draw(bkg_spr);

            if (view_.get_center().x < 0.f) {
                bkg_spr.setPosition(-256, 256);
                window.draw(bkg_spr);
            } else if (view_.get_center().x > right_x_wrap_threshold) {
                bkg_spr.setPosition(256, 256);
                window.draw(bkg_spr);
            }
        }
    }

    view.setCenter(view_.get_center().x + view_.get_size().x / 2,
                   view_.get_center().y + view_.get_size().y / 2);
    window.setView(view);

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

    window.draw(sf::Sprite(::platform->data()->map_0_rt_.getTexture()));
    window.draw(sf::Sprite(::platform->data()->map_1_rt_.getTexture()));


    for (auto& spr : reversed(::draw_queue)) {
        if (spr.get_alpha() == Sprite::Alpha::transparent) {
            continue;
        }
        const Vec2<Float>& pos = spr.get_position();
        const Vec2<bool>& flip = spr.get_flip();

        sf::Sprite sf_spr;

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
            window.draw(sf_spr, &shader);
        } else {
            window.draw(sf_spr);
        }
    }


    ::platform->data()->fade_overlay_.setPosition(
        {view_.get_center().x, view_.get_center().y});

    window.draw(::platform->data()->fade_overlay_);

    view.setCenter({view_.get_size().x / 2, view_.get_size().y / 2});

    window.setView(view);

    window.draw(::platform->data()->overlay_);

    window.display();

    draw_queue.clear();
}


void Platform::Screen::fade(Float amount,
                            ColorConstant k,
                            std::optional<ColorConstant> base)
{
    const auto c = real_color(k);

    if (not base) {
        ::platform->data()->fade_overlay_.setFillColor(
            {static_cast<uint8_t>(c.x * 255),
             static_cast<uint8_t>(c.y * 255),
             static_cast<uint8_t>(c.z * 255),
             static_cast<uint8_t>(amount * 255)});
    } else {
        const auto c2 = real_color(*base);
        ::platform->data()->fade_overlay_.setFillColor(
            {interpolate(static_cast<uint8_t>(c.x * 255),
                         static_cast<uint8_t>(c2.x * 255),
                         amount),
             interpolate(static_cast<uint8_t>(c.y * 255),
                         static_cast<uint8_t>(c2.y * 255),
                         amount),
             interpolate(static_cast<uint8_t>(c.z * 255),
                         static_cast<uint8_t>(c2.z * 255),
                         amount),
             static_cast<uint8_t>(255)});
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


Platform::Speaker::Speaker()
{
    // TODO...
}


void Platform::Speaker::play_music(const char* name,
                                   bool loop,
                                   Microseconds offset)
{
    // TODO...
}


void Platform::Speaker::stop_music()
{
    // TODO...
}


void Platform::Speaker::play_sound(const char* name, int priority)
{
    // TODO...
}


bool is_sound_playing(const char* name)
{
    return false; // TODO...
}


////////////////////////////////////////////////////////////////////////////////
// Logger
////////////////////////////////////////////////////////////////////////////////


static std::ofstream logfile("logfile.txt");


void Platform::Logger::log(Logger::Severity level, const char* msg)
{
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
               << msg << std::endl;
    };

    write_msg(logfile);
    write_msg(std::cout);
}


Platform::Logger::Logger()
{
}


////////////////////////////////////////////////////////////////////////////////
// Platform
////////////////////////////////////////////////////////////////////////////////


static std::unordered_map<std::string, sf::Keyboard::Key> key_lookup{
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


Platform::Platform()
{
    ::platform = this;

    push_task(&::watchdog_task);

    data_ = new Data(*this);
    if (not data_) {
        error(*this, "Failed to allocate context");
        exit(EXIT_FAILURE);
    }


    screen_.view_.set_size(screen_.size().cast<Float>());


    auto shader_folder =
        Conf(*::platform).expect<Conf::String>("paths", "shader_folder");


    if (not data_->color_shader_.loadFromFile(
            shader_folder.c_str() + std::string("colorShader.frag"),
            sf::Shader::Fragment)) {
        error(*this, "Failed to load shader");
    }
    data_->color_shader_.setUniform("texture", sf::Shader::CurrentTexture);

    data_->fade_overlay_.setSize(
        {Float(screen_.size().x), Float(screen_.size().y)});


    data_->map_0_rt_.create(16 * 32, 20 * 24);
    data_->map_1_rt_.create(16 * 32, 20 * 24);
    data_->background_rt_.create(32 * 8, 32 * 8);

#define CONF_KEY(KEY)                                                          \
    keymap[static_cast<int>(Key::KEY)] =                                       \
        ::key_lookup[Conf(*::platform)                                         \
                         .expect<Conf::String>("keyboard-bindings", #KEY)      \
                         .c_str()];

    CONF_KEY(left);
    CONF_KEY(right);
    CONF_KEY(up);
    CONF_KEY(down);
    CONF_KEY(action_1);
    CONF_KEY(action_2);
    CONF_KEY(alt_1);
    CONF_KEY(alt_2);
    CONF_KEY(start);
    CONF_KEY(select);
}


void Platform::soft_exit()
{
    data()->window_.close();
}


static const char* const save_file_name = "save.dat";


bool Platform::write_save(const PersistentData& data)
{
    std::ofstream out(save_file_name,
                      std::ios_base::out | std::ios_base::binary);

    out.write(reinterpret_cast<const char*>(&data), sizeof data);

    return true;
}


std::optional<PersistentData> Platform::read_save()
{
    std::ifstream in(save_file_name, std::ios_base::in | std::ios_base::binary);

    if (!in) {
        return {};
    }

    PersistentData save;
    in.read(reinterpret_cast<char*>(&save), sizeof save);

    return save;
}


static std::vector<std::thread> worker_threads;


void Platform::push_task(Task* task)
{
    worker_threads.emplace_back([task] {
        while (not task->complete()) {
            task->run();
        }
    });
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
    std::lock_guard<std::mutex> guard(texture_swap_mutex);
    texture_swap_requests.push({TextureSwap::spritesheet, name});
}


void Platform::load_tile0_texture(const char* name)
{
    std::lock_guard<std::mutex> guard(texture_swap_mutex);
    texture_swap_requests.push({TextureSwap::tile0, name});
}


void Platform::load_tile1_texture(const char* name)
{
    std::lock_guard<std::mutex> guard(texture_swap_mutex);
    texture_swap_requests.push({TextureSwap::tile1, name});
}


void Platform::load_overlay_texture(const char* name)
{
    std::lock_guard<std::mutex> guard(texture_swap_mutex);
    texture_swap_requests.push({TextureSwap::overlay, name});
}


void Platform::on_watchdog_timeout(WatchdogCallback callback)
{
    // TODO
}


std::map<Layer, std::map<std::pair<u16, u16>, TileDesc>> tile_layers_;


void Platform::set_tile(Layer layer, u16 x, u16 y, TileDesc val)
{
    tile_layers_[layer][{x, y}] = val;

    std::lock_guard<std::mutex> guard(::tile_swap_mutex);
    ::tile_swap_requests.push({layer, x, y, val});
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

    std::lock_guard<std::mutex> guard(::tile_swap_mutex);

    for (int i = 0; i < ::platform->data()->overlay_.size().x; ++i) {
        for (int j = 0; j < ::platform->data()->overlay_.size().y; ++j) {
            ::tile_swap_requests.push({Layer::overlay, i, j, tile_desc});
        }
    }
}


void Platform::set_overlay_origin(s16 x, s16 y)
{
    // TODO
}


void Platform::enable_glyph_mode(bool enabled)
{
    // TODO
}


TileDesc Platform::map_glyph(const utf8::Codepoint& glyph,
                             TextureCpMapper mapper)
{
    if (auto mapping = mapper(glyph)) {
        // TODO...
        return 111;
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


static std::string config_data;


const char* Platform::config_data() const
{
    if (::config_data.empty()) {
        std::fstream file("../config.ini");
        std::stringstream buffer;
        buffer << file.rdbuf();
        ::config_data = buffer.str();
    }
    return ::config_data.c_str();
}


void start(Platform&);

int main()
{
    random_seed() = time(nullptr);

    Platform pf;
    start(pf);

    for (auto& worker : worker_threads) {
        worker.join();
    }
}

#ifdef _WIN32
int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main();
}
#endif


void SynchronizedBase::init(Platform& pf)
{
    impl_ = new std::mutex;
    if (not impl_) {
        error(pf, "failed to allocate mutex");
    }
}


void SynchronizedBase::lock()
{
    reinterpret_cast<std::mutex*>(impl_)->lock();
}


void SynchronizedBase::unlock()
{
    reinterpret_cast<std::mutex*>(impl_)->unlock();
}


SynchronizedBase::~SynchronizedBase()
{
    delete reinterpret_cast<std::mutex*>(impl_);
}


////////////////////////////////////////////////////////////////////////////////
// Stopwatch
////////////////////////////////////////////////////////////////////////////////


Platform::Stopwatch::Stopwatch()
{
    impl_ = nullptr;
}


void Platform::Stopwatch::start()
{
}


int Platform::Stopwatch::stop()
{
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Misc
////////////////////////////////////////////////////////////////////////////////


// There was a reason to have platform specific versions of these functions,
// some embedded systems do not support division in hardware, so it needs to be
// implemented in software or via BIOS calls. For desktop systems, though, we
// don't need to worry so much about the speed of a division operation, at least
// not to the same extent as for older 32bit microcontrollers.
s32 fast_divide(s32 numerator, s32 denominator)
{
    return numerator / denominator;
}


s32 fast_mod(s32 numerator, s32 denominator)
{
    return numerator % denominator;
}
