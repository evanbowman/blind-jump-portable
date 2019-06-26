#pragma once

#include <array>
#include "numeric.hpp"


// Anything platform specific should be defined here, and implemented in
// platform.cpp.



////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


using Microseconds = u32;


class DeltaClock {
public:

    DeltaClock();

    Microseconds reset();

private:
    void* impl_;
};



////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////


class Keyboard {
public:
    
    Keyboard();
    
    enum Key {
        action_1,
        action_2,
        start,
        left,
        right,
        up,
        down,
        count
    };
    
    void poll();

    template <Key k>
    bool pressed() const
    {
        return states_[k];
    }
    
private:
    std::array<bool, Key::count> states_;
};



////////////////////////////////////////////////////////////////////////////////
// Sprite
////////////////////////////////////////////////////////////////////////////////


class Sprite {
public:
    enum class Shape {
        square,
        wide,
        tall
    };

    enum class Size {

    };

    Sprite();
    Sprite(const Sprite&) = delete;
    ~Sprite();

    bool initialize();

    void set_position(const Vec2<float>& position);

    const Vec2<float> get_position() const;

    void* native_handle() const;

private:
    void* data_;
};



////////////////////////////////////////////////////////////////////////////////
// Screen
////////////////////////////////////////////////////////////////////////////////


class Screen {
public:

    Screen();

    void draw(const Sprite& spr);

    void clear();
    
    void display();

    const Vec2<u32>& size() const;
    
private:    
    void* userdata_;
};



////////////////////////////////////////////////////////////////////////////////
// Platform
////////////////////////////////////////////////////////////////////////////////


class Platform {
public:
    
    Platform();

    Screen& screen();

    Keyboard& keyboard();

private:
    Screen screen_;
    Keyboard keyboard_;
};
