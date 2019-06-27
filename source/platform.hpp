#pragma once

#include <array>
#include "numeric.hpp"


// Anything platform specific should be defined here, and implemented
// in platform.cpp. Ideally, when porting the game to a new system,
// this is the only file that should need to change.



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
    enum class Size {
        square_8_8,
        square_16_16,
        square_32_32,
        square_64_64,
        wide_16_8,
        wide_32_8,
        wide_32_16,
        wide_64_32,
        tall_8_16,
        tall_8_32,
        tall_16_32,
        tall_32_64,
    };

    Sprite();
    Sprite(const Sprite&) = delete;
    ~Sprite();

    bool initialize(Size size, u32 keyframe);

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

    inline Screen& screen()
    {
        return screen_;
    }

    inline Keyboard& keyboard()
    {
        return keyboard_;
    }

private:
    Screen screen_;
    Keyboard keyboard_;
};
