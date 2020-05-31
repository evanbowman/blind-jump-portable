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
#include <fstream>
#include <sstream>
#include <queue>
#include <chrono>
#include <thread>


static sf::RenderWindow* window = nullptr;
static sf::RenderTexture* rt = nullptr;


class Platform::Data {
public:
    sf::Texture spritesheet_;
    sf::Texture tileset_;
    sf::Shader color_shader_;
    sf::RectangleShape fade_overlay_;
};


static Platform* platform = nullptr;


////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


DeltaClock::DeltaClock() : impl_(new sf::Clock)
{
}


std::chrono::time_point throttle_start = std::chrono::high_resolution_clock::now();
std::chrono::time_point throttle_stop = std::chrono::high_resolution_clock::now();


Microseconds DeltaClock::reset()
{
    // NOTE: I originally developed the game on the nintendo gameboy
    // advance. The game was designed to use delta timing, but stuff still seems
    // to break when running on modern hardware, which has a WAY faster clock
    // than the gameboy. So for now, I'm intentionally slowing things down.

    throttle_stop = std::chrono::high_resolution_clock::now();

    const auto gba_fixed_step = 2000;

    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(throttle_stop - throttle_start);
    if (elapsed.count() < gba_fixed_step) {
        std::this_thread::sleep_for(std::chrono::microseconds((gba_fixed_step - 1000) - elapsed.count()));
    }


    auto val = reinterpret_cast<sf::Clock*>(impl_)->restart().asMicroseconds();


    throttle_start = std::chrono::high_resolution_clock::now();;


    return val;
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


void Platform::Keyboard::poll()
{
    // FIXME: poll is now called from the logic thread, which means that the
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
            ::window->close();
            break;

        case sf::Event::KeyPressed:
            switch (event.key.code) {
            case sf::Keyboard::Left:
                states_[size_t(Key::left)] = true;
                break;

            case sf::Keyboard::Right:
                states_[size_t(Key::right)] = true;
                break;

            case sf::Keyboard::Up:
                states_[size_t(Key::up)] = true;
                break;

            case sf::Keyboard::Down:
                states_[size_t(Key::down)] = true;
                break;

            case sf::Keyboard::X:
                states_[size_t(Key::action_1)] = true;
                break;

            case sf::Keyboard::Y:
                states_[size_t(Key::action_2)] = true;
                break;

            default:
                break;
            }
            break;

        case sf::Event::KeyReleased:
            switch (event.key.code) {
            case sf::Keyboard::Left:
                states_[size_t(Key::left)] = false;
                break;

            case sf::Keyboard::Right:
                states_[size_t(Key::right)] = false;
                break;

            case sf::Keyboard::Up:
                states_[size_t(Key::up)] = false;
                break;

            case sf::Keyboard::Down:
                states_[size_t(Key::down)] = false;
                break;

            case sf::Keyboard::X:
                states_[size_t(Key::action_1)] = false;
                break;

            case sf::Keyboard::Y:
                states_[size_t(Key::action_2)] = false;
                break;

            default:
                break;
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

#include <iostream>
Platform::Screen::Screen()
{
    if (not::window) {
        ::window = new sf::RenderWindow(sf::VideoMode(240*4, 160*4), "Blind Jump");
        if (not::window) {
            exit(EXIT_FAILURE);
        }
        ::window->setVerticalSyncEnabled(true);

        ::rt = new sf::RenderTexture();
    }
    view_.set_size(this->size().cast<Float>());
    std::cout << this->size().x << " " << this->size().y << std::endl;
}


Vec2<u32> Platform::Screen::size() const
{
    return {::window->getSize().x / 4, ::window->getSize().y / 4};
}


void Platform::Screen::clear()
{
    ::window->clear();


    std::lock_guard<std::mutex> guard(::event_queue_lock);

    sf::Event event;
    while (::window->pollEvent(event)) {
        ::event_queue.push(event);
    }
}


std::vector<Sprite> draw_queue;


static sf::Glsl::Vec3 real_color(ColorConstant k)
{
    switch (k) {
    case ColorConstant::spanish_crimson:
        static const sf::Glsl::Vec3 spn_crimson(
            29.f / 31.f, 3.f / 31.f, 11.f / 31.f);
        return spn_crimson;

    case ColorConstant::electric_blue:
        static const sf::Glsl::Vec3 el_blue(
            9.f / 31.f, 31.f / 31.f, 31.f / 31.f);
        return el_blue;

    // case ColorConstant::coquelicot:
    //     static const sf::Glsl::Vec3 coquelicot(
    //         30.f / 31.f, 7.f / 31.f, 1.f / 31.f);
    //     return coquelicot;

    default:
    case ColorConstant::null:
    case ColorConstant::rich_black:
        static const sf::Glsl::Vec3 rich_black(0.f, 0.f, 2.f / 31.f);
        return rich_black;
    }
}


void Platform::Screen::display()
{
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

        sf_spr.setTexture(::platform->data()->spritesheet_);

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
            ::window->draw(sf_spr, &shader);
        } else {
            ::window->draw(sf_spr);
        }
    }

    sf::View view;
    view.setCenter(view_.get_center().x + view_.get_size().x / 2,
                   view_.get_center().y + view_.get_size().y / 2);
    view.setSize(view_.get_size().x, view_.get_size().y);

    ::platform->data()->fade_overlay_.setPosition(
        {view_.get_center().x, view_.get_center().y});

    ::window->draw(::platform->data()->fade_overlay_);

    ::window->setView(view);
    ::window->display();

    draw_queue.clear();
}


void Platform::Screen::fade(Float amount, ColorConstant k, std::optional<ColorConstant> base)
{
    const auto& c = real_color(k);

    ::platform->data()->fade_overlay_.setFillColor(
        {static_cast<uint8_t>(c.x * 255),
         static_cast<uint8_t>(c.y * 255),
         static_cast<uint8_t>(c.z * 255),
         static_cast<uint8_t>(amount * 255)});
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
    logfile << msg << std::endl;
}


Platform::Logger::Logger()
{
}


////////////////////////////////////////////////////////////////////////////////
// Platform
////////////////////////////////////////////////////////////////////////////////


Platform::Platform()
{
    data_ = new Data;
    if (not data_) {
        error(*this, "Failed to allocate context");
        exit(EXIT_FAILURE);
    }
    sf::Image image;
    if (not image.loadFromFile("../images/spritesheet.png")) {
        error(*this, "Failed to load spritesheet");
    }
    image.createMaskFromColor({255, 0, 255, 255});
    if (not data_->spritesheet_.loadFromImage(image)) {
        error(*this, "Failed to create spritesheet texture");
        exit(EXIT_FAILURE);
    }
    if (not data_->color_shader_.loadFromFile("../shaders/colorShader.frag",
                                              sf::Shader::Fragment)) {
        error(*this, "Failed to load shader");
    }
    data_->color_shader_.setUniform("texture", sf::Shader::CurrentTexture);

    data_->fade_overlay_.setSize(
        {Float(screen_.size().x), Float(screen_.size().y)});

    ::platform = this;
}


std::optional<PersistentData> Platform::read_save()
{
    // TODO...
    return {};
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
    return ::window->isOpen();
}


void Platform::sleep(u32 frames)
{
    // TODO...
}


void Platform::load_sprite_texture(const char* name)
{
}


void Platform::load_tile0_texture(const char* name)
{
}


void Platform::load_tile1_texture(const char* name)
{
}


void Platform::load_overlay_texture(const char* name)
{
}


bool Platform::write_save(const PersistentData& data)
{
    return true; // TODO
}


void Platform::on_watchdog_timeout(WatchdogCallback callback)
{
    // TODO
}


std::map<Layer, std::map<std::pair<u16, u16>, TileDesc>> tile_layers_;


void Platform::set_tile(Layer layer, u16 x, u16 y, TileDesc val)
{
    tile_layers_[layer][{x, y}] = val;
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
}


void Platform::set_overlay_origin(s16 x, s16 y)
{
    // TODO
}


void Platform::enable_glyph_mode(bool enabled)
{
    // TODO
}


TileDesc Platform::map_glyph(const utf8::Codepoint& glyph, TextureCpMapper)
{
    // TODO
    return 111;
}


void Platform::fatal()
{
    exit(1);
}


void Platform::feed_watchdog()
{
    // TODO
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
