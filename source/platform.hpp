#pragma once

#include "numeric.hpp"
#include "tileMap.hpp"
#include <array>


// Anything platform specific should be defined here.


////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


class DeltaClock {
public:
    static DeltaClock& instance()
    {
        static DeltaClock inst;
        return inst;
    }

    ~DeltaClock();

    Microseconds reset();

private:
    DeltaClock();

    void* impl_;
};


////////////////////////////////////////////////////////////////////////////////
// Keyboard
////////////////////////////////////////////////////////////////////////////////


class Keyboard {
public:
    Keyboard()
    {
        for (int i = 0; i < Key::count; ++i) {
            states_[i] = false;
            prev_[i] = false;
        }
    }

    enum Key { action_1, action_2, start, left, right, up, down, count };

    void poll();

    template <Key k> bool pressed() const
    {
        return states_[k];
    }

    template <Key k> bool down_transition() const
    {
        return states_[k] and not prev_[k];
    }

    template <Key k> bool up_transition() const
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

    static constexpr u32 sprite_limit = 128;

    void draw(const Sprite& spr);

    void clear();

    void display();

    Vec2<u32> size() const;

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

    // Sleep halts the game for an amount of time equal to some number
    // of game updates. Given that the game should be running at
    // 60fps, one update equals 1/60 of a second.
    void sleep(u32 frames);

    bool is_running() const;

private:
    Screen screen_;
    Keyboard keyboard_;
};


template <u32 N> inline int random_choice(Platform& pfrm)
{
    return pfrm.random() * N >> (8 * sizeof(int) - 1);
}

inline int random_choice(Platform& pfrm, u32 n)
{
    return pfrm.random() * n >> (8 * sizeof(int) - 1);
}
