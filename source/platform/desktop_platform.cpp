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

static sf::RenderWindow* window = nullptr;


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


Microseconds DeltaClock::reset()
{
    return reinterpret_cast<sf::Clock*>(impl_)->restart().asMicroseconds();
}


DeltaClock::~DeltaClock()
{
    delete reinterpret_cast<sf::Clock*>(impl_);
}


////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////


void Platform::Keyboard::poll()
{
    for (size_t i = 0; i < size_t(Key::count); ++i) {
        prev_[i] = states_[i];
    }
    sf::Event event;
    while (::window->pollEvent(event)) {
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

            case sf::Keyboard::A:
                states_[size_t(Key::action_1)] = true;
                break;

            case sf::Keyboard::B:
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

            case sf::Keyboard::A:
                states_[size_t(Key::action_1)] = false;
                break;

            case sf::Keyboard::B:
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


Platform::Screen::Screen()
{
    if (not::window) {
        ::window = new sf::RenderWindow(sf::VideoMode(480, 320), "SFML window");
        if (not::window) {
            exit(EXIT_FAILURE);
        }
        ::window->setVerticalSyncEnabled(true);
    }
    view_.set_size(this->size().cast<Float>());
}


Vec2<u32> Platform::Screen::size() const
{
    return {::window->getSize().x, ::window->getSize().y};
}


void Platform::Screen::clear()
{
    ::window->clear();
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

    case ColorConstant::coquelicot:
        static const sf::Glsl::Vec3 coquelicot(
            30.f / 31.f, 7.f / 31.f, 1.f / 31.f);
        return coquelicot;

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


void Platform::Screen::fade(Float amount, ColorConstant k)
{
    const auto& c = real_color(k);

    ::platform->data()->fade_overlay_.setFillColor(
        {static_cast<uint8_t>(c.x * 255),
         static_cast<uint8_t>(c.y * 255),
         static_cast<uint8_t>(c.z * 255),
         static_cast<uint8_t>(amount * 255)});
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


void Platform::Speaker::play(Note note, Octave o, Channel c)
{
    // TODO...
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
    if (not image.loadFromFile("../spritesheet.png")) {
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


#include <chrono>
#include <thread>


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


void Platform::push_map(const TileMap& map)
{
    for (int i = 0; i < TileMap::width; ++i) {
        for (int j = 0; j < TileMap::height; ++j) {
        }
    }
}


void Platform::sleep(u32 frames)
{
    // TODO...
}


void Platform::load_sprite_texture(const char* name)
{
}


void Platform::load_tile_texture(const char* name)
{
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
