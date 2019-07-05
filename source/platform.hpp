#pragma once

#include <array>
#include "numeric.hpp"
#include "tileMap.hpp"


// Anything platform specific should be defined here, and implemented
// in platform.cpp. Ideally, when porting the game to a new system,
// this is the only file that should need to change.



////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


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
// Screen
////////////////////////////////////////////////////////////////////////////////

#include "sprite.hpp"
#include "view.hpp"


class Screen {
public:

    Screen();

    static constexpr u32 sprite_limit = 256;

    enum class DisplayMode {
        normal, translucent
    };

    void draw(const Sprite& spr);

    void clear();

    void display();

    const Vec2<u32>& size() const;

    void set_view(const View& view)
    {
        view_ = view;
    }

    const View& get_view() const
    {
        return view_;
    }

private:
    View view_;
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

    void push_map(const TileMap& map);

    int random();

    void collect_entropy();

private:
    Screen screen_;
    Keyboard keyboard_;
};


template <u32 N>
inline int random_choice(Platform& pfrm)
{
    return pfrm.random() * N >> 15;
}
