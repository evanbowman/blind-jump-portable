#include "platform.hpp"

////////////////////////////////////////////////////////////////////////////////
//
//
// Desktop Platform
//
//
////////////////////////////////////////////////////////////////////////////////


#ifndef __GBA__

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>


static sf::RenderWindow* window = nullptr;


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


void Keyboard::poll()
{
    for (size_t i = 0; i < Key::count; ++i) {
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
                states_[left] = true;
                break;

            case sf::Keyboard::Right:
                states_[right] = true;
                break;

            case sf::Keyboard::Up:
                states_[up] = true;
                break;

            case sf::Keyboard::Down:
                states_[down] = true;
                break;

            default:
                break;
            }
            break;

        case sf::Event::KeyReleased:
            switch (event.key.code) {
            case sf::Keyboard::Left:
                states_[left] = true;
                break;

            case sf::Keyboard::Right:
                states_[right] = true;
                break;

            case sf::Keyboard::Up:
                states_[up] = true;
                break;

            case sf::Keyboard::Down:
                states_[down] = true;
                break;

            default:
                break;
            }
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// Screen
////////////////////////////////////////////////////////////////////////////////


Screen::Screen()
{
    if (::window) {
        throw std::runtime_error("Only one screen allowed at a time");
    }
    ::window = new sf::RenderWindow(sf::VideoMode(800, 600), "SFML window");
}


Vec2<u32> Screen::size() const
{
    return {::window->getSize().x, ::window->getSize().y};
}


void Screen::clear()
{
    ::window->clear();
}


void Screen::display()
{
    ::window->display();
}


void Screen::draw(const Sprite& spr)
{
    const Vec2<Float>& pos = spr.get_position();
    const Vec2<bool>& flip = spr.get_flip();

    sf::Sprite sf_spr;

    sf_spr.setPosition({pos.x, pos.y});

    if (spr.get_alpha() == Sprite::Alpha::translucent) {
        sf_spr.setColor({255, 255, 255, 128});
    }

    sf_spr.setScale({flip.x ? -1.f : 1.f, flip.y ? -1.f : 1.f});

    // TODO: Remap TextureIndex -> sfml sprite subrect
}


////////////////////////////////////////////////////////////////////////////////
// Platform
////////////////////////////////////////////////////////////////////////////////


#include <random>

std::mt19937 rng{std::random_device()()};


int Platform::random()
{
    return rng();
}


Platform::Platform()
{
}


bool Platform::is_running() const
{
    return ::window->isOpen();
}


void Platform::push_map(const TileMap& map)
{
    // ...
}


void Platform::sleep(u32 frames)
{
    // TODO...
}


#endif // __GBA__
