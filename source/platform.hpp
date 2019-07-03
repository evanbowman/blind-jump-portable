#pragma once

#include <array>
#include "numeric.hpp"


// Anything platform specific should be defined here, and implemented
// in platform.cpp. Ideally, when porting the game to a new system,
// this is the only file that should need to change.



////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


using Microseconds = s32;


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

    template <Key k>
    bool down_transition() const
    {
        return states_[k] and not prev_[k];
    }

    template <Key k>
    bool up_transition() const
    {
        return not states_[k] and prev_[k];
    }

private:
    std::array<bool, Key::count> prev_;
    std::array<bool, Key::count> states_;
};



////////////////////////////////////////////////////////////////////////////////
// Sprite
////////////////////////////////////////////////////////////////////////////////


class Sprite {
public:

    Sprite();
    Sprite(const Sprite&) = delete;
    ~Sprite();

    bool initialize();

    void set_position(const Vec2<float>& position);

    void set_texture_index(u32 texture_index);

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
